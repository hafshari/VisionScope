#pragma once

#include "visionscope/model/frame.hpp"
#include "visionscope/view/i_frame_accelerator.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace visionscope::view {

[[nodiscard]] std::string base64_encode(const std::uint8_t* data, std::size_t len);

[[nodiscard]] std::vector<std::uint8_t> encode_png_rgb24(const model::Frame& frame);

[[nodiscard]] std::string build_iterm2_inline_sequence(
    const model::Frame& frame, int cell_w, int cell_h,
    const IFrameAccelerator& accel);

[[nodiscard]] std::string build_kitty_rgb_sequence(const model::Frame& frame,
                                                   int cell_w, int cell_h,
                                                   const IFrameAccelerator& accel);

}  // namespace visionscope::view
