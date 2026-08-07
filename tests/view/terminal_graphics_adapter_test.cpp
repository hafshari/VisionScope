#include "visionscope/view/software_frame_accelerator.hpp"
#include "visionscope/view/terminal_graphics_adapter.hpp"

#include <gtest/gtest.h>

using visionscope::model::Frame;
using visionscope::model::GraphicsProtocol;
using visionscope::model::TerminalCapabilities;
using visionscope::view::SoftwareFrameAccelerator;
using visionscope::view::TerminalGraphicsAdapter;

TEST(TerminalGraphicsAdapterTest, ReportsConfiguredProtocolFromCaps) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kKitty;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};
  EXPECT_EQ(adapter.configured_protocol(), GraphicsProtocol::kKitty);
  EXPECT_STREQ(adapter.accel_backend_name(), "software");
}

TEST(TerminalGraphicsAdapterTest, DrawingHonorsKittyAndITerm2) {
  TerminalCapabilities caps;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};
  EXPECT_EQ(adapter.drawing_protocol(GraphicsProtocol::kKitty),
            GraphicsProtocol::kKitty);
  EXPECT_EQ(adapter.drawing_protocol(GraphicsProtocol::kITerm2),
            GraphicsProtocol::kITerm2);
  EXPECT_EQ(adapter.drawing_protocol(GraphicsProtocol::kUnicode),
            GraphicsProtocol::kUnicode);
  EXPECT_EQ(adapter.drawing_protocol(GraphicsProtocol::kSixel),
            GraphicsProtocol::kUnicode);
}

TEST(TerminalGraphicsAdapterTest, PresentBitmapRejectsUnicode) {
  TerminalCapabilities caps;
  SoftwareFrameAccelerator accel;
  TerminalGraphicsAdapter adapter{caps, accel};
  Frame frame;
  frame.width = 1;
  frame.height = 1;
  frame.pixels = {1, 2, 3};
  EXPECT_FALSE(adapter.present_bitmap(frame, GraphicsProtocol::kUnicode, 1, 1, 10,
                                      10));
}
