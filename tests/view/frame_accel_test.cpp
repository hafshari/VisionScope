#include "visionscope/view/make_frame_accelerator.hpp"
#include "visionscope/view/software_frame_accelerator.hpp"

#include <gtest/gtest.h>

using visionscope::model::Frame;
using visionscope::model::PixelFormat;
using visionscope::view::make_frame_accelerator;
using visionscope::view::SoftwareFrameAccelerator;

namespace {

Frame solid(int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  Frame frame;
  frame.width = w;
  frame.height = h;
  frame.format = PixelFormat::kRgb24;
  frame.pixels.assign(static_cast<std::size_t>(w * h * 3), 0);
  for (int i = 0; i < w * h; ++i) {
    frame.pixels[static_cast<std::size_t>(i * 3 + 0)] = r;
    frame.pixels[static_cast<std::size_t>(i * 3 + 1)] = g;
    frame.pixels[static_cast<std::size_t>(i * 3 + 2)] = b;
  }
  return frame;
}

}  // namespace

TEST(FrameAccelTest, SoftwareBackendLabel) {
  SoftwareFrameAccelerator accel;
  EXPECT_STREQ(accel.backend_name(), "software");
  EXPECT_TRUE(accel.encode_iterm_image(solid(8, 8, 1, 2, 3)).empty());
}

TEST(FrameAccelTest, SoftwareDownscaleRespectsBounds) {
  SoftwareFrameAccelerator accel;
  const auto out = accel.downscale_rgb24(solid(100, 80, 9, 9, 9), 40, 30);
  EXPECT_EQ(out.width, 40);
  EXPECT_EQ(out.height, 30);
  EXPECT_FALSE(out.empty());
}

TEST(FrameAccelTest, FactoryReturnsUsableAccelerator) {
  auto accel = make_frame_accelerator();
  ASSERT_NE(accel, nullptr);
  EXPECT_NE(accel->backend_name()[0], '\0');
  const auto out = accel->downscale_rgb24(solid(50, 40, 3, 4, 5), 25, 20);
  EXPECT_EQ(out.width, 25);
  EXPECT_EQ(out.height, 20);
}

#if defined(VISIONSCOPE_HAS_APPLE_FRAME_ACCEL)
TEST(FrameAccelTest, AppleFactoryUsesAccelerateBackend) {
  auto accel = make_frame_accelerator();
  ASSERT_NE(accel, nullptr);
  EXPECT_STREQ(accel->backend_name(), "Accelerate+ImageIO");
}

TEST(FrameAccelTest, AppleITermEncodeProducesJpeg) {
  auto accel = make_frame_accelerator();
  ASSERT_NE(accel, nullptr);
  const auto jpeg = accel->encode_iterm_image(solid(32, 24, 10, 20, 30));
  ASSERT_GE(jpeg.size(), 4u);
  EXPECT_EQ(jpeg[0], 0xff);
  EXPECT_EQ(jpeg[1], 0xd8);
}
#endif
