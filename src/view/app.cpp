#include "visionscope/view/app.hpp"

#include "visionscope/model/terminal_capabilities.hpp"
#include "visionscope/view/terminal_graphics_adapter.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/terminal.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace visionscope::view {
namespace {

int index_of_protocol(const std::vector<model::GraphicsProtocol>& available,
                      model::GraphicsProtocol active) {
  for (int i = 0; i < static_cast<int>(available.size()); ++i) {
    if (available[static_cast<std::size_t>(i)] == active) {
      return i;
    }
  }
  return 0;
}

bool is_bitmap_protocol(model::GraphicsProtocol protocol) {
  return protocol == model::GraphicsProtocol::kITerm2 ||
         protocol == model::GraphicsProtocol::kKitty;
}

std::string truncate_ascii(std::string text, int max_cols) {
  if (max_cols <= 0) {
    return {};
  }
  if (static_cast<int>(text.size()) <= max_cols) {
    return text;
  }
  if (max_cols <= 3) {
    return text.substr(0, static_cast<std::size_t>(max_cols));
  }
  text.resize(static_cast<std::size_t>(max_cols - 3));
  text += "...";
  return text;
}

// Chrome around the preview. Must match the vbox in the renderer exactly:
//   top: 1 title · 4 menus · 1 meta · 1 status · 1 separator
//   bottom: 1 buttons (Ascii, single row — below the bitmap paint region)
// Screen row 1 is the top border; content starts at row 2.
constexpr int kBorderThickness = 1;
constexpr int kTopChromeContentRows = 8;
constexpr int kBottomChromeContentRows = 1;
constexpr int kPreviewOriginRow =
    kBorderThickness + 1 + kTopChromeContentRows;
constexpr int kPreviewOriginCol = kBorderThickness + 2;
// 1-based CUP rows for ANSI chrome patches (must match renderer vbox).
constexpr int kMetaScreenRow = kBorderThickness + 1 + 1 + 4;  // after title+menus
constexpr int kStatusScreenRow = kMetaScreenRow + 1;

struct PreviewGeom {
  int origin_row = kPreviewOriginRow;
  int origin_col = kPreviewOriginCol;
  int cols = 40;
  int rows = 10;
};

PreviewGeom compute_preview_geom(int term_cols, int term_rows) {
  PreviewGeom geom;
  geom.origin_row = kPreviewOriginRow;
  geom.origin_col = kPreviewOriginCol;
  geom.cols = std::max(20, term_cols - 2 * kBorderThickness - 2);
  const int reserved = 2 * kBorderThickness + kTopChromeContentRows +
                       kBottomChromeContentRows;
  geom.rows = std::max(4, term_rows - reserved);
  // Never let the bitmap paint into the bottom button row / border.
  const int last_preview_row = term_rows - kBorderThickness - kBottomChromeContentRows;
  if (geom.origin_row + geom.rows - 1 > last_preview_row) {
    geom.rows = std::max(4, last_preview_row - geom.origin_row + 1);
  }
  return geom;
}

}  // namespace

int run_app(viewmodel::CameraViewModel& view_model, IFrameAccelerator& accel) {
  using namespace ftxui;

  TerminalGraphicsAdapter graphics(view_model.capabilities(), accel);
  auto screen = ScreenInteractive::Fullscreen();

  int selected = view_model.selected_index();
  std::vector<std::string> entries;
  for (const auto& device : view_model.devices()) {
    entries.push_back(device.name + " [" + device.id + "]");
  }
  if (entries.empty()) {
    entries.emplace_back("(no cameras detected)");
  }

  auto menu = Menu(&entries, &selected);

  int protocol_selected = index_of_protocol(view_model.available_protocols(),
                                            view_model.active_protocol());
  std::vector<std::string> protocol_entries;
  for (const auto protocol : view_model.available_protocols()) {
    protocol_entries.emplace_back(model::graphics_protocol_name(protocol));
  }
  auto protocol_menu = Menu(&protocol_entries, &protocol_selected);

  model::GraphicsProtocol drawing_for_overlay = model::GraphicsProtocol::kUnicode;
  PreviewGeom overlay_geom{};
  std::atomic<bool> bitmap_mode{false};
  auto last_bitmap_present = std::chrono::steady_clock::time_point{};

  // Ascii = true single-row labels. Simple/Border use a box border (3 rows)
  // and vanish when the parent row is height-clamped to 1.
  const auto button_opt = ButtonOption::Ascii();
  auto buttons = Container::Horizontal({
      Button("Refresh",
             [&] {
               view_model.refresh_devices();
               entries.clear();
               for (const auto& device : view_model.devices()) {
                 entries.push_back(device.name + " [" + device.id + "]");
               }
               if (entries.empty()) {
                 entries.emplace_back("(no cameras detected)");
               }
               selected = view_model.selected_index();
             },
             button_opt),
      Button("Start",
             [&] {
               view_model.select_index(selected);
               (void)view_model.start();
             },
             button_opt),
      Button("Stop", [&] { view_model.stop(); }, button_opt),
      Button("Quit", screen.ExitLoopClosure(), button_opt),
  });

  auto menus = Container::Horizontal({menu, protocol_menu});
  auto layout = Container::Vertical({
      buttons,
      menus,
  });

  auto present_overlay = [&](bool force = false) {
    if (!is_bitmap_protocol(drawing_for_overlay)) {
      return;
    }
    const auto& frame = view_model.latest_frame();
    if (frame.empty()) {
      return;
    }
    // Cap encode/terminal writes ~12 Hz. Capture still runs at 30 Hz; presenting
    // every wake was dominated by PNG/OSC cost on iTerm2.
    const auto now = std::chrono::steady_clock::now();
    if (!force && last_bitmap_present != std::chrono::steady_clock::time_point{} &&
        now - last_bitmap_present < std::chrono::milliseconds(80)) {
      return;
    }
    last_bitmap_present = now;
    (void)graphics.present_bitmap(frame, drawing_for_overlay,
                                  overlay_geom.origin_col,
                                  overlay_geom.origin_row, overlay_geom.cols,
                                  overlay_geom.rows);
  };

  // Patch FPS/status without an FTXUI redraw (redraws wipe Kitty/iTerm2 images).
  auto paint_chrome_stats = [&] {
    if (!bitmap_mode.load(std::memory_order_relaxed)) {
      return;
    }
    const auto drawing = drawing_for_overlay;
    const std::string stats = view_model.stats_label();
    std::string meta = "active=" + view_model.active_protocol_label() +
                       " · drawing=" + model::graphics_protocol_name(drawing) +
                       " · " + stats + " · accel=" + graphics.accel_backend_name() +
                       " · " +
                       view_model.capabilities().detected_terminal;
    meta = truncate_ascii(std::move(meta), overlay_geom.cols);
    std::string status_line =
        truncate_ascii("Status: " + view_model.status(), overlay_geom.cols);

    // Pad/clear within the content width so stale characters do not linger.
    if (static_cast<int>(meta.size()) < overlay_geom.cols) {
      meta.append(static_cast<std::size_t>(overlay_geom.cols - static_cast<int>(meta.size())),
                  ' ');
    }
    if (static_cast<int>(status_line.size()) < overlay_geom.cols) {
      status_line.append(
          static_cast<std::size_t>(overlay_geom.cols -
                                   static_cast<int>(status_line.size())),
          ' ');
    }

    std::cout << "\033[s"
              << "\033[" << kMetaScreenRow << ";" << kPreviewOriginCol << "H"
              << meta
              << "\033[" << kStatusScreenRow << ";" << kPreviewOriginCol << "H"
              << status_line << "\033[u" << std::flush;
  };

  auto renderer = Renderer(layout, [&] {
    // Capture runs on the refresh thread; avoid double-grab here.
    if (selected != view_model.selected_index() && !view_model.running()) {
      view_model.select_index(selected);
    }

    const auto& available = view_model.available_protocols();
    if (!available.empty() && protocol_selected >= 0 &&
        protocol_selected < static_cast<int>(available.size())) {
      const auto chosen = available[static_cast<std::size_t>(protocol_selected)];
      if (chosen != view_model.active_protocol()) {
        (void)view_model.select_protocol(chosen);
      }
    }

    const auto term = Terminal::Size();
    overlay_geom = compute_preview_geom(term.dimx, term.dimy);

    const auto& latest = view_model.latest_frame();
    const auto active = view_model.active_protocol();
    const auto drawing = graphics.drawing_protocol(active);
    drawing_for_overlay = drawing;
    bitmap_mode.store(is_bitmap_protocol(drawing), std::memory_order_relaxed);

    auto preview = graphics.preview_element(latest, overlay_geom.cols,
                                            overlay_geom.rows, active) |
                   size(WIDTH, EQUAL, overlay_geom.cols) |
                   size(HEIGHT, EQUAL, overlay_geom.rows);

    const std::string stats = view_model.stats_label();
    std::string meta = "active=" + view_model.active_protocol_label() +
                       " · drawing=" + model::graphics_protocol_name(drawing) +
                       " · " + stats + " · accel=" + graphics.accel_backend_name() +
                       " · " +
                       view_model.capabilities().detected_terminal;
    meta = truncate_ascii(std::move(meta), overlay_geom.cols);

    std::string status_line =
        truncate_ascii("Status: " + view_model.status(), overlay_geom.cols);

    // Preview in the middle; Ascii controls on their own bottom row so the
    // Kitty/iTerm2 overlay cannot cover them and labels are never border-clipped.
    return vbox({
               hbox({
                   text("VisionScope") | bold,
                   filler(),
                   text(stats) | dim,
               }) | size(HEIGHT, EQUAL, 1),
               hbox({
                   vbox({
                       text("Cameras") | bold,
                       menu->Render() | ftxui::frame | size(HEIGHT, EQUAL, 3),
                   }) | flex,
                   separator(),
                   vbox({
                       text("Graphics") | bold,
                       protocol_menu->Render() | ftxui::frame |
                           size(HEIGHT, EQUAL, 3),
                   }) | flex,
               }) | size(HEIGHT, EQUAL, 4),
               text(meta) | dim | size(HEIGHT, EQUAL, 1),
               text(status_line) | size(HEIGHT, EQUAL, 1),
               separator(),
               preview,
               buttons->Render() | size(HEIGHT, EQUAL, 1),
           }) |
           border;
  });

  auto component = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Character('Q')) {
      screen.Exit();
      return true;
    }
    if (event == Event::Character('g') || event == Event::Character('G')) {
      view_model.cycle_protocol();
      protocol_selected = index_of_protocol(view_model.available_protocols(),
                                            view_model.active_protocol());
      return true;
    }
    return false;
  });

  std::atomic<bool> running{true};
  std::atomic<bool> capture_wake{false};
  std::thread refresh([&] {
    while (running.load(std::memory_order_relaxed)) {
      // Capture cadence is protocol-independent so `cap` FPS stays stable when
      // toggling graphics. Display cost still differs (Unicode redraw vs PNG/OSC),
      // but grab rate no longer follows the slower bitmap path.
      std::this_thread::sleep_for(std::chrono::milliseconds(33));

      if (!running.load(std::memory_order_relaxed)) {
        break;
      }

      const bool bitmap = bitmap_mode.load(std::memory_order_relaxed);
      screen.Post([&] {
        view_model.tick();
        capture_wake.store(true, std::memory_order_relaxed);
      });

      if (!bitmap) {
        // Unicode needs an FTXUI redraw to show half-blocks.
        screen.PostEvent(Event::Custom);
      }
    }
  });

  Loop loop(&screen, component);
  while (!loop.HasQuitted()) {
    loop.RunOnceBlocking();
    // Capture-only wakes throttle encodes; other wakes (keys/menus) force a
    // repaint because FTXUI Draw clears Kitty/iTerm2 images.
    const bool from_capture =
        capture_wake.exchange(false, std::memory_order_relaxed);
    present_overlay(/*force=*/!from_capture);
    paint_chrome_stats();
  }

  running = false;
  refresh.join();
  view_model.stop();
  return 0;
}

}  // namespace visionscope::view
