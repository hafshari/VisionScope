#pragma once

#include "visionscope/model/frame.hpp"

#include <cstdint>
#include <vector>

namespace visionscope::view {

// Port for scale/encode acceleration (Strategy). View code depends on this
// abstraction; platform backends live under src/view/platform/<os>/.
class IFrameAccelerator {
 public:
  virtual ~IFrameAccelerator() = default;

  [[nodiscard]] virtual const char* backend_name() const = 0;

  [[nodiscard]] virtual model::Frame downscale_rgb24(const model::Frame& frame,
                                                    int max_w,
                                                    int max_h) const = 0;

  // Optional still-image encode for iTerm2 (e.g. JPEG). Empty → caller uses PNG.
  [[nodiscard]] virtual std::vector<std::uint8_t> encode_iterm_image(
      const model::Frame& frame) const = 0;
};

}  // namespace visionscope::view
