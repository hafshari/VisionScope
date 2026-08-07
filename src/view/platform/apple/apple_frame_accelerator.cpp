#include "apple_frame_accelerator.hpp"

#include <Accelerate/Accelerate.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

#include <algorithm>
#include <cstdint>

namespace visionscope::view::platform::apple {
namespace {

int bytes_per_pixel(model::PixelFormat format) {
  return format == model::PixelFormat::kRgba32 ? 4 : 3;
}

}  // namespace

const char* AppleFrameAccelerator::backend_name() const {
  return "Accelerate+ImageIO";
}

model::Frame AppleFrameAccelerator::downscale_rgb24(const model::Frame& frame,
                                                    int max_w,
                                                    int max_h) const {
  if (frame.empty() || max_w <= 0 || max_h <= 0) {
    return {};
  }
  if (frame.format != model::PixelFormat::kRgb24 &&
      frame.format != model::PixelFormat::kRgba32) {
    return software_.downscale_rgb24(frame, max_w, max_h);
  }

  const int dst_w = std::min(frame.width, max_w);
  const int dst_h = std::min(frame.height, max_h);
  if (dst_w == frame.width && dst_h == frame.height &&
      frame.format == model::PixelFormat::kRgb24) {
    return frame;
  }

  const int src_bpp = bytes_per_pixel(frame.format);
  const std::size_t src_argb_bytes =
      static_cast<std::size_t>(frame.width) * frame.height * 4;
  const std::size_t dst_argb_bytes =
      static_cast<std::size_t>(dst_w) * dst_h * 4;

  std::vector<std::uint8_t> src_argb(src_argb_bytes);
  std::vector<std::uint8_t> dst_argb(dst_argb_bytes);

  vImage_Buffer src_a{
      src_argb.data(),
      static_cast<vImagePixelCount>(frame.height),
      static_cast<vImagePixelCount>(frame.width),
      static_cast<size_t>(frame.width * 4),
  };

  vImage_Error err = kvImageNoError;
  if (frame.format == model::PixelFormat::kRgb24) {
    vImage_Buffer src_rgb{
        const_cast<std::uint8_t*>(frame.pixels.data()),
        static_cast<vImagePixelCount>(frame.height),
        static_cast<vImagePixelCount>(frame.width),
        static_cast<size_t>(frame.width * src_bpp),
    };
    err = vImageConvert_RGB888toARGB8888(&src_rgb, nullptr, 255, &src_a, false,
                                         kvImageNoFlags);
  } else {
    for (int i = 0; i < frame.width * frame.height; ++i) {
      const auto s = static_cast<std::size_t>(i * 4);
      const auto d = static_cast<std::size_t>(i * 4);
      if (s + 3 >= frame.pixels.size()) {
        break;
      }
      src_argb[d + 0] = frame.pixels[s + 3];
      src_argb[d + 1] = frame.pixels[s + 0];
      src_argb[d + 2] = frame.pixels[s + 1];
      src_argb[d + 3] = frame.pixels[s + 2];
    }
  }
  if (err != kvImageNoError) {
    return software_.downscale_rgb24(frame, max_w, max_h);
  }

  vImage_Buffer dst_a{
      dst_argb.data(),
      static_cast<vImagePixelCount>(dst_h),
      static_cast<vImagePixelCount>(dst_w),
      static_cast<size_t>(dst_w * 4),
  };
  err = vImageScale_ARGB8888(&src_a, &dst_a, nullptr, kvImageNoFlags);
  if (err != kvImageNoError) {
    return software_.downscale_rgb24(frame, max_w, max_h);
  }

  model::Frame out;
  out.width = dst_w;
  out.height = dst_h;
  out.format = model::PixelFormat::kRgb24;
  out.pixels.resize(static_cast<std::size_t>(dst_w * dst_h * 3));
  vImage_Buffer dst_rgb{
      out.pixels.data(),
      static_cast<vImagePixelCount>(dst_h),
      static_cast<vImagePixelCount>(dst_w),
      static_cast<size_t>(dst_w * 3),
  };
  err = vImageConvert_ARGB8888toRGB888(&dst_a, &dst_rgb, kvImageNoFlags);
  if (err != kvImageNoError) {
    return software_.downscale_rgb24(frame, max_w, max_h);
  }
  return out;
}

std::vector<std::uint8_t> AppleFrameAccelerator::encode_iterm_image(
    const model::Frame& frame) const {
  std::vector<std::uint8_t> out;
  if (frame.empty() || frame.format != model::PixelFormat::kRgb24) {
    return out;
  }

  CGDataProviderRef provider = CGDataProviderCreateWithData(
      nullptr, frame.pixels.data(), frame.pixels.size(), nullptr);
  if (provider == nullptr) {
    return out;
  }

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
  if (color_space == nullptr) {
    CGDataProviderRelease(provider);
    return out;
  }

  CGImageRef image = CGImageCreate(
      static_cast<size_t>(frame.width), static_cast<size_t>(frame.height), 8, 24,
      static_cast<size_t>(frame.width * 3), color_space,
      static_cast<CGBitmapInfo>(static_cast<uint32_t>(kCGBitmapByteOrderDefault) |
                                static_cast<uint32_t>(kCGImageAlphaNone)),
      provider, nullptr, false, kCGRenderingIntentDefault);
  CGDataProviderRelease(provider);
  CGColorSpaceRelease(color_space);
  if (image == nullptr) {
    return out;
  }

  CFMutableDataRef data = CFDataCreateMutable(kCFAllocatorDefault, 0);
  if (data == nullptr) {
    CGImageRelease(image);
    return out;
  }

  CGImageDestinationRef dest = CGImageDestinationCreateWithData(
      data, CFSTR("public.jpeg"), 1, nullptr);
  if (dest == nullptr) {
    CFRelease(data);
    CGImageRelease(image);
    return out;
  }

  const void* keys[] = {kCGImageDestinationLossyCompressionQuality};
  const float quality = 0.55f;
  CFNumberRef quality_num =
      CFNumberCreate(kCFAllocatorDefault, kCFNumberFloatType, &quality);
  const void* values[] = {quality_num};
  CFDictionaryRef opts = CFDictionaryCreate(
      kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  CFRelease(quality_num);

  CGImageDestinationAddImage(dest, image, opts);
  CFRelease(opts);
  const bool ok = CGImageDestinationFinalize(dest);
  CFRelease(dest);
  CGImageRelease(image);

  if (ok) {
    const auto* bytes = CFDataGetBytePtr(data);
    const auto len = static_cast<std::size_t>(CFDataGetLength(data));
    out.assign(bytes, bytes + len);
  }
  CFRelease(data);
  return out;
}

}  // namespace visionscope::view::platform::apple
