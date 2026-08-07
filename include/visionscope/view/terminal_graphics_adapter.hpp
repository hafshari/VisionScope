#pragma once

#include "visionscope/model/frame.hpp"
#include "visionscope/model/terminal_capabilities.hpp"
#include "visionscope/view/i_frame_accelerator.hpp"

#include <ftxui/dom/elements.hpp>

#include <cstdint>
#include <vector>

namespace visionscope::view {

struct RgbColor {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

struct HalfBlockCell {
  RgbColor top;
  RgbColor bottom;
};

class TerminalGraphicsAdapter {
 public:
  TerminalGraphicsAdapter(const model::TerminalCapabilities& caps,
                          const IFrameAccelerator& accel);

  // Emits Kitty/iTerm2 escape sequences to stdout at (cell_x, cell_y) (1-based).
  // Returns false if the frame is empty or the protocol is not drawable this way.
  [[nodiscard]] bool present_bitmap(const model::Frame& frame,
                                    model::GraphicsProtocol protocol, int cell_x,
                                    int cell_y, int cell_w, int cell_h) const;

  [[nodiscard]] std::vector<std::vector<HalfBlockCell>> rasterize(
      const model::Frame& frame, int cols, int rows) const;

  // Unicode half-blocks, or a blank reserved box for bitmap protocols.
  [[nodiscard]] ftxui::Element preview_element(
      const model::Frame& frame, int cols, int rows,
      model::GraphicsProtocol requested) const;

  [[nodiscard]] model::GraphicsProtocol drawing_protocol(
      model::GraphicsProtocol requested) const;

  [[nodiscard]] model::GraphicsProtocol configured_protocol() const;

  [[nodiscard]] const char* accel_backend_name() const;

 private:
  model::TerminalCapabilities caps_;
  const IFrameAccelerator& accel_;
};

}  // namespace visionscope::view
