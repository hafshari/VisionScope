#pragma once

#include "visionscope/view/i_frame_accelerator.hpp"
#include "visionscope/view/software_frame_accelerator.hpp"

namespace visionscope::view::platform::apple {

// Accelerate.framework vImage scale + ImageIO JPEG for iTerm2.
class AppleFrameAccelerator final : public IFrameAccelerator {
 public:
  [[nodiscard]] const char* backend_name() const override;
  [[nodiscard]] model::Frame downscale_rgb24(const model::Frame& frame, int max_w,
                                            int max_h) const override;
  [[nodiscard]] std::vector<std::uint8_t> encode_iterm_image(
      const model::Frame& frame) const override;

 private:
  SoftwareFrameAccelerator software_;
};

}  // namespace visionscope::view::platform::apple
