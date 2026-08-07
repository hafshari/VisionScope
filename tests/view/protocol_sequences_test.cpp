#include "visionscope/view/protocol_sequences.hpp"
#include "visionscope/view/software_frame_accelerator.hpp"

#include <gtest/gtest.h>

#include <string>

using visionscope::model::Frame;
using visionscope::model::PixelFormat;
using visionscope::view::base64_encode;
using visionscope::view::build_iterm2_inline_sequence;
using visionscope::view::build_kitty_rgb_sequence;
using visionscope::view::encode_png_rgb24;
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

TEST(ProtocolSequencesTest, Base64EncodesKnownVector) {
  const std::uint8_t data[] = {'M', 'a', 'n'};
  EXPECT_EQ(base64_encode(data, 3), "TWFu");
}

TEST(ProtocolSequencesTest, PngHasSignatureAndChunks) {
  const auto png = encode_png_rgb24(solid(2, 2, 10, 20, 30));
  ASSERT_GE(png.size(), 8u);
  EXPECT_EQ(png[0], 137);
  EXPECT_EQ(png[1], 80);
  EXPECT_EQ(png[2], 78);
  EXPECT_EQ(png[3], 71);
  const std::string as_bytes(png.begin(), png.end());
  EXPECT_NE(as_bytes.find("IHDR"), std::string::npos);
  EXPECT_NE(as_bytes.find("IDAT"), std::string::npos);
  EXPECT_NE(as_bytes.find("IEND"), std::string::npos);
}

TEST(ProtocolSequencesTest, ITerm2SequenceContainsOsc1337) {
  SoftwareFrameAccelerator accel;
  const auto seq =
      build_iterm2_inline_sequence(solid(4, 4, 1, 2, 3), 20, 10, accel);
  ASSERT_FALSE(seq.empty());
  EXPECT_NE(seq.find("\033]1337;File=inline=1"), std::string::npos);
  EXPECT_NE(seq.find(";width=20;height=10"), std::string::npos);
  EXPECT_NE(seq.find("doNotMoveCursor=1"), std::string::npos);
}

TEST(ProtocolSequencesTest, KittySequenceContainsGraphicsPrefix) {
  SoftwareFrameAccelerator accel;
  const auto seq =
      build_kitty_rgb_sequence(solid(4, 4, 1, 2, 3), 20, 10, accel);
  ASSERT_FALSE(seq.empty());
  EXPECT_NE(seq.find("\033_Ga=T,f=24,o=z,i=1"), std::string::npos);
  EXPECT_NE(seq.find(",c=20,r=10"), std::string::npos);
}

TEST(ProtocolSequencesTest, CompressedPngSmallerThanRawRgb) {
  const auto png = encode_png_rgb24(solid(64, 48, 40, 80, 120));
  ASSERT_FALSE(png.empty());
  EXPECT_LT(png.size(), static_cast<std::size_t>(64 * 48 * 3));
}

TEST(ProtocolSequencesTest, SoftwareDownscaleRespectsMaxBounds) {
  SoftwareFrameAccelerator accel;
  const auto out = accel.downscale_rgb24(solid(100, 80, 9, 9, 9), 40, 30);
  EXPECT_EQ(out.width, 40);
  EXPECT_EQ(out.height, 30);
  EXPECT_FALSE(out.empty());
}
