#pragma once

#include <cstdint>
#include <vector>

namespace visionscope::model {

enum class PixelFormat {
  kRgb24,
  kRgba32,
};

struct Frame {
  int width = 0;
  int height = 0;
  PixelFormat format = PixelFormat::kRgb24;
  std::vector<std::uint8_t> pixels;

  [[nodiscard]] bool empty() const noexcept {
    return width <= 0 || height <= 0 || pixels.empty();
  }
};

}  // namespace visionscope::model
