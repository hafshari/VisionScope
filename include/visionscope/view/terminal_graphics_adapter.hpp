#pragma once

#include "visionscope/model/frame.hpp"
#include "visionscope/model/terminal_capabilities.hpp"

namespace visionscope::view {

class TerminalGraphicsAdapter {
 public:
  explicit TerminalGraphicsAdapter(const model::TerminalCapabilities& caps);

  [[nodiscard]] bool render_frame(const model::Frame& frame, int cell_x, int cell_y,
                                  int cell_w, int cell_h) const;

  [[nodiscard]] model::GraphicsProtocol active_protocol() const;

 private:
  model::TerminalCapabilities caps_;
};

}  // namespace visionscope::view
