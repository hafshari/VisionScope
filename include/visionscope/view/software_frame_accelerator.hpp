#pragma once

#include "visionscope/view/i_frame_accelerator.hpp"

namespace visionscope::view {

// Portable CPU nearest-neighbor scale; PNG path left to protocol_sequences.
class SoftwareFrameAccelerator final : public IFrameAccelerator {
 public:
  [[nodiscard]] const char* backend_name() const override;
  [[nodiscard]] model::Frame downscale_rgb24(const model::Frame& frame, int max_w,
                                            int max_h) const override;
  [[nodiscard]] std::vector<std::uint8_t> encode_iterm_image(
      const model::Frame& frame) const override;
};

}  // namespace visionscope::view
