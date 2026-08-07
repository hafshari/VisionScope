#include "visionscope/view/software_frame_accelerator.hpp"
#include "visionscope/view/terminal_graphics_adapter.hpp"

#include <gtest/gtest.h>

using visionscope::model::Frame;
using visionscope::model::GraphicsProtocol;
using visionscope::model::PixelFormat;
using visionscope::model::TerminalCapabilities;
using visionscope::view::SoftwareFrameAccelerator;
using visionscope::view::TerminalGraphicsAdapter;

namespace {

Frame solid_rgb(int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
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

Frame vertical_split(int w, int h, std::uint8_t top_v, std::uint8_t bottom_v) {
  Frame frame;
  frame.width = w;
  frame.height = h;
  frame.format = PixelFormat::kRgb24;
  frame.pixels.assign(static_cast<std::size_t>(w * h * 3), 0);
  const int mid = h / 2;
  for (int y = 0; y < h; ++y) {
    const std::uint8_t v = y < mid ? top_v : bottom_v;
    for (int x = 0; x < w; ++x) {
      const auto i = static_cast<std::size_t>((y * w + x) * 3);
      frame.pixels[i + 0] = v;
      frame.pixels[i + 1] = v;
      frame.pixels[i + 2] = v;
    }
  }
  return frame;
}

}  // namespace

TEST(HalfBlockRasterTest, EmptyFrameYieldsEmptyGrid) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kUnicode;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};

  Frame empty;
  const auto grid = adapter.rasterize(empty, 10, 5);
  EXPECT_TRUE(grid.empty());
}

TEST(HalfBlockRasterTest, SolidFrameFillsCells) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kUnicode;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};

  const auto grid = adapter.rasterize(solid_rgb(4, 4, 200, 10, 30), 2, 2);
  ASSERT_EQ(grid.size(), 2u);
  ASSERT_EQ(grid[0].size(), 2u);
  EXPECT_EQ(grid[0][0].top.r, 200);
  EXPECT_EQ(grid[0][0].top.g, 10);
  EXPECT_EQ(grid[0][0].top.b, 30);
  EXPECT_EQ(grid[1][1].bottom.r, 200);
}

TEST(HalfBlockRasterTest, HalfBlockSeparatesTopAndBottom) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kUnicode;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};

  // 2x2 frame, 1 cell: top row dark, bottom row bright → ▀ with dark fg / bright bg
  const auto grid = adapter.rasterize(vertical_split(2, 2, 0, 255), 1, 1);
  ASSERT_EQ(grid.size(), 1u);
  ASSERT_EQ(grid[0].size(), 1u);
  EXPECT_LT(grid[0][0].top.r, 40);
  EXPECT_GT(grid[0][0].bottom.r, 215);
}
