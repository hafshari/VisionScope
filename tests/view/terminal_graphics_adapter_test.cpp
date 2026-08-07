#include "visionscope/view/terminal_graphics_adapter.hpp"

#include <gtest/gtest.h>

using visionscope::model::Frame;
using visionscope::model::GraphicsProtocol;
using visionscope::model::TerminalCapabilities;
using visionscope::view::TerminalGraphicsAdapter;

TEST(TerminalGraphicsAdapterTest, ReportsActiveProtocolFromCaps) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kKitty;
  TerminalGraphicsAdapter adapter{caps};
  EXPECT_EQ(adapter.active_protocol(), GraphicsProtocol::kKitty);
}

TEST(TerminalGraphicsAdapterTest, RenderFrameRequiresNonEmptyFrame) {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kITerm2;
  TerminalGraphicsAdapter adapter{caps};

  Frame empty;
  EXPECT_FALSE(adapter.render_frame(empty, 0, 0, 10, 10));

  Frame frame;
  frame.width = 1;
  frame.height = 1;
  frame.pixels = {0, 0, 0};
  EXPECT_TRUE(adapter.render_frame(frame, 0, 0, 10, 10));
}
