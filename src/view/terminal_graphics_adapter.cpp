#include "visionscope/view/terminal_graphics_adapter.hpp"

#include "visionscope/view/protocol_sequences.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>

namespace visionscope::view {
namespace {

int bytes_per_pixel(model::PixelFormat format) {
  return format == model::PixelFormat::kRgba32 ? 4 : 3;
}

RgbColor sample_rect(const model::Frame& frame, int x0, int y0, int x1, int y1) {
  x0 = std::clamp(x0, 0, frame.width);
  x1 = std::clamp(x1, 0, frame.width);
  y0 = std::clamp(y0, 0, frame.height);
  y1 = std::clamp(y1, 0, frame.height);
  if (x1 <= x0) {
    x1 = std::min(frame.width, x0 + 1);
  }
  if (y1 <= y0) {
    y1 = std::min(frame.height, y0 + 1);
  }

  const int bpp = bytes_per_pixel(frame.format);
  std::uint64_t sum_r = 0;
  std::uint64_t sum_g = 0;
  std::uint64_t sum_b = 0;
  std::uint64_t count = 0;

  for (int y = y0; y < y1; ++y) {
    for (int x = x0; x < x1; ++x) {
      const auto i =
          static_cast<std::size_t>((y * frame.width + x) * bpp);
      if (i + 2 >= frame.pixels.size()) {
        continue;
      }
      sum_r += frame.pixels[i + 0];
      sum_g += frame.pixels[i + 1];
      sum_b += frame.pixels[i + 2];
      ++count;
    }
  }

  if (count == 0) {
    return {};
  }
  return RgbColor{
      static_cast<std::uint8_t>(sum_r / count),
      static_cast<std::uint8_t>(sum_g / count),
      static_cast<std::uint8_t>(sum_b / count),
  };
}

ftxui::Element blank_preview(int cols, int rows) {
  using namespace ftxui;
  Elements lines;
  lines.reserve(static_cast<std::size_t>(rows));
  const std::string blank(static_cast<std::size_t>(std::max(cols, 1)), ' ');
  for (int i = 0; i < rows; ++i) {
    lines.push_back(text(blank));
  }
  return vbox(std::move(lines));
}

}  // namespace

TerminalGraphicsAdapter::TerminalGraphicsAdapter(
    const model::TerminalCapabilities& caps, const IFrameAccelerator& accel)
    : caps_(caps), accel_(accel) {}

model::GraphicsProtocol TerminalGraphicsAdapter::configured_protocol() const {
  return caps_.preferred;
}

const char* TerminalGraphicsAdapter::accel_backend_name() const {
  return accel_.backend_name();
}

model::GraphicsProtocol TerminalGraphicsAdapter::drawing_protocol(
    model::GraphicsProtocol requested) const {
  switch (requested) {
    case model::GraphicsProtocol::kKitty:
    case model::GraphicsProtocol::kITerm2:
    case model::GraphicsProtocol::kUnicode:
      return requested;
    case model::GraphicsProtocol::kSixel:
    case model::GraphicsProtocol::kNone:
      return model::GraphicsProtocol::kUnicode;
  }
  return model::GraphicsProtocol::kUnicode;
}

bool TerminalGraphicsAdapter::present_bitmap(const model::Frame& frame,
                                             model::GraphicsProtocol protocol,
                                             int cell_x, int cell_y, int cell_w,
                                             int cell_h) const {
  if (frame.empty() || cell_w <= 0 || cell_h <= 0 || cell_x <= 0 || cell_y <= 0) {
    return false;
  }

  std::string sequence;
  if (protocol == model::GraphicsProtocol::kITerm2) {
    sequence = build_iterm2_inline_sequence(frame, cell_w, cell_h, accel_);
  } else if (protocol == model::GraphicsProtocol::kKitty) {
    sequence = build_kitty_rgb_sequence(frame, cell_w, cell_h, accel_);
  } else {
    return false;
  }
  if (sequence.empty()) {
    return false;
  }

  // Save/restore cursor so bitmap paint does not disturb FTXUI's cursor state.
  std::cout << "\033[s"
            << "\033[" << cell_y << ";" << cell_x << "H" << sequence << "\033[u"
            << std::flush;
  return true;
}

std::vector<std::vector<HalfBlockCell>> TerminalGraphicsAdapter::rasterize(
    const model::Frame& frame, int cols, int rows) const {
  std::vector<std::vector<HalfBlockCell>> grid;
  if (frame.empty() || cols <= 0 || rows <= 0) {
    return grid;
  }

  grid.resize(static_cast<std::size_t>(rows));
  const int virtual_h = rows * 2;

  for (int row = 0; row < rows; ++row) {
    auto& line = grid[static_cast<std::size_t>(row)];
    line.resize(static_cast<std::size_t>(cols));
    for (int col = 0; col < cols; ++col) {
      const int x0 = col * frame.width / cols;
      const int x1 = (col + 1) * frame.width / cols;
      const int top_y0 = (row * 2) * frame.height / virtual_h;
      const int top_y1 = (row * 2 + 1) * frame.height / virtual_h;
      const int bot_y0 = (row * 2 + 1) * frame.height / virtual_h;
      const int bot_y1 = (row * 2 + 2) * frame.height / virtual_h;

      HalfBlockCell cell;
      cell.top = sample_rect(frame, x0, top_y0, x1, top_y1);
      cell.bottom = sample_rect(frame, x0, bot_y0, x1, bot_y1);
      line[static_cast<std::size_t>(col)] = cell;
    }
  }
  return grid;
}

ftxui::Element TerminalGraphicsAdapter::preview_element(
    const model::Frame& frame, int cols, int rows,
    model::GraphicsProtocol requested) const {
  using namespace ftxui;

  const auto drawing = drawing_protocol(requested);
  if (drawing == model::GraphicsProtocol::kITerm2 ||
      drawing == model::GraphicsProtocol::kKitty) {
    // Reserve cells; present_bitmap paints after FTXUI flush.
    return blank_preview(cols, rows);
  }

  if (frame.empty() || cols <= 0 || rows <= 0) {
    return text("No frame yet — select a camera and press Start") | dim;
  }

  const auto grid = rasterize(frame, cols, rows);
  Elements lines;
  lines.reserve(grid.size());
  for (const auto& row : grid) {
    Elements cells;
    cells.reserve(row.size());
    for (const auto& cell : row) {
      cells.push_back(text("▀") |
                      color(Color::RGB(cell.top.r, cell.top.g, cell.top.b)) |
                      bgcolor(Color::RGB(cell.bottom.r, cell.bottom.g,
                                         cell.bottom.b)));
    }
    lines.push_back(hbox(std::move(cells)));
  }
  return vbox(std::move(lines));
}

}  // namespace visionscope::view
