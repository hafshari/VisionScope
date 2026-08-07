#include "visionscope/view/terminal_graphics_adapter.hpp"

namespace visionscope::view {

TerminalGraphicsAdapter::TerminalGraphicsAdapter(
    const model::TerminalCapabilities& caps)
    : caps_(caps) {}

model::GraphicsProtocol TerminalGraphicsAdapter::active_protocol() const {
  return caps_.preferred;
}

bool TerminalGraphicsAdapter::render_frame(const model::Frame& frame, int /*cell_x*/,
                                           int /*cell_y*/, int /*cell_w*/,
                                           int /*cell_h*/) const {
  return !frame.empty() && caps_.preferred != model::GraphicsProtocol::kNone;
}

}  // namespace visionscope::view
