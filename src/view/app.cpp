#include "visionscope/view/app.hpp"

#include "visionscope/view/terminal_graphics_adapter.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace visionscope::view {

int run_app(viewmodel::CameraViewModel& view_model) {
  using namespace ftxui;

  TerminalGraphicsAdapter graphics(view_model.capabilities());
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

  auto buttons = Container::Horizontal({
      Button("Refresh", [&] {
        view_model.refresh_devices();
        entries.clear();
        for (const auto& device : view_model.devices()) {
          entries.push_back(device.name + " [" + device.id + "]");
        }
        if (entries.empty()) {
          entries.emplace_back("(no cameras detected)");
        }
        selected = view_model.selected_index();
      }),
      Button("Start", [&] {
        view_model.select_index(selected);
        (void)view_model.start();
      }),
      Button("Stop", [&] { view_model.stop(); }),
      Button("Quit", screen.ExitLoopClosure()),
  });

  auto layout = Container::Vertical({
      menu,
      buttons,
  });

  auto renderer = Renderer(layout, [&] {
    view_model.tick();
    if (selected != view_model.selected_index() && !view_model.running()) {
      view_model.select_index(selected);
    }

    const auto latest = view_model.latest_frame();
    const bool would_draw = graphics.render_frame(latest, 0, 0, 80, 24);

    std::string preview =
        "Preview protocol: " + view_model.preferred_protocol_label();
    preview += " | terminal: " + view_model.capabilities().detected_terminal;
    if (!view_model.capabilities().override_source.empty()) {
      preview += " (override=" + view_model.capabilities().override_source + ")";
    }
    preview += "\n";
    if (latest.empty()) {
      preview += "No frame yet (scaffold: Start opens camera when available).";
    } else {
      preview += "Frame " + std::to_string(latest.width) + "x" +
                std::to_string(latest.height) +
                (would_draw ? " ready for graphics adapter"
                            : " held (no protocol draw in scaffold)");
    }

    return vbox({
               text("VisionScope") | bold | center,
               text("TUI webcam preview (scaffold)") | dim | center,
               separator(),
               text("Cameras") | bold,
               menu->Render() | ftxui::frame | flex,
               separator(),
               paragraph(preview) | flex,
               separator(),
               text("Status: " + view_model.status()),
               text("Keys: ↑/↓ select · Tab focus · q quit"),
               buttons->Render(),
           }) |
           border;
  });

  auto component = CatchEvent(renderer, [&](Event event) {
    if (event == Event::Character('q') || event == Event::Character('Q')) {
      screen.Exit();
      return true;
    }
    return false;
  });

  std::atomic<bool> running{true};
  std::thread refresh([&screen, &running] {
    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      screen.PostEvent(Event::Custom);
    }
  });

  screen.Loop(component);
  running = false;
  refresh.join();
  view_model.stop();
  return 0;
}

}  // namespace visionscope::view
