#include "visionscope/view/software_frame_accelerator.hpp"

#include <algorithm>

namespace visionscope::view {
namespace {

int bytes_per_pixel(model::PixelFormat format) {
  return format == model::PixelFormat::kRgba32 ? 4 : 3;
}

}  // namespace

const char* SoftwareFrameAccelerator::backend_name() const {
  return "software";
}

model::Frame SoftwareFrameAccelerator::downscale_rgb24(
    const model::Frame& frame, int max_w, int max_h) const {
  model::Frame out;
  if (frame.empty() || max_w <= 0 || max_h <= 0) {
    return out;
  }

  const int dst_w = std::min(frame.width, max_w);
  const int dst_h = std::min(frame.height, max_h);
  out.width = dst_w;
  out.height = dst_h;
  out.format = model::PixelFormat::kRgb24;
  out.pixels.resize(static_cast<std::size_t>(dst_w * dst_h * 3));

  const int bpp = bytes_per_pixel(frame.format);
  for (int y = 0; y < dst_h; ++y) {
    const int sy = y * frame.height / dst_h;
    for (int x = 0; x < dst_w; ++x) {
      const int sx = x * frame.width / dst_w;
      const auto si =
          static_cast<std::size_t>((sy * frame.width + sx) * bpp);
      const auto di = static_cast<std::size_t>((y * dst_w + x) * 3);
      if (si + 2 >= frame.pixels.size()) {
        continue;
      }
      out.pixels[di + 0] = frame.pixels[si + 0];
      out.pixels[di + 1] = frame.pixels[si + 1];
      out.pixels[di + 2] = frame.pixels[si + 2];
    }
  }
  return out;
}

std::vector<std::uint8_t> SoftwareFrameAccelerator::encode_iterm_image(
    const model::Frame& /*frame*/) const {
  // Software path uses encode_png_rgb24 in protocol_sequences.
  return {};
}

}  // namespace visionscope::view
