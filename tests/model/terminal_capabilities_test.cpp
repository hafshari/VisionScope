#include "fakes/fake_environment.hpp"

#include "visionscope/model/terminal_capabilities.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using visionscope::model::GraphicsProtocol;
using visionscope::model::TerminalCapabilityDetector;
using visionscope::model::available_graphics_protocols;
using visionscope::test::FakeEnvironment;

TEST(TerminalCapabilitiesTest, CliOverrideSetsPreferredButKeepsDetectedFlags) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "kitty");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect("iterm2");

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kITerm2);
  EXPECT_TRUE(caps.kitty);
  EXPECT_TRUE(caps.iterm2);
  EXPECT_EQ(caps.override_source, "cli");
  EXPECT_EQ(caps.detected_terminal, "kitty");
}

TEST(TerminalCapabilitiesTest, AppEnvVarDoesNotOverrideProtocol) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "iTerm.app");
  env.set("ITERM_SESSION_ID", "w0t0:0");
  env.set("VISIONSCOPE_GRAPHICS", "kitty");

  const auto caps =
      visionscope::model::detect_terminal_capabilities(env, nullptr);

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kITerm2);
  EXPECT_TRUE(caps.override_source.empty());
  EXPECT_FALSE(caps.kitty);
}

TEST(TerminalCapabilitiesTest, WezTermPrefersKittyAmongMultiProtocolSupport) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "WezTerm");
  env.set("WEZTERM_PANE", "1");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect();

  EXPECT_TRUE(caps.kitty);
  EXPECT_TRUE(caps.iterm2);
  EXPECT_TRUE(caps.sixel);
  EXPECT_EQ(caps.preferred, GraphicsProtocol::kKitty);
  EXPECT_EQ(caps.detected_terminal, "WezTerm");

  const auto available = available_graphics_protocols(caps);
  ASSERT_GE(available.size(), 4u);
  EXPECT_EQ(available.front(), GraphicsProtocol::kUnicode);
  EXPECT_NE(std::find(available.begin(), available.end(), GraphicsProtocol::kKitty),
            available.end());
  EXPECT_NE(std::find(available.begin(), available.end(), GraphicsProtocol::kITerm2),
            available.end());
  EXPECT_NE(std::find(available.begin(), available.end(), GraphicsProtocol::kSixel),
            available.end());
}

TEST(TerminalCapabilitiesTest, ITerm2DetectedViaTermProgram) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "iTerm.app");
  env.set("ITERM_SESSION_ID", "w0t0:0");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect();

  EXPECT_TRUE(caps.iterm2);
  EXPECT_EQ(caps.preferred, GraphicsProtocol::kITerm2);
}

TEST(TerminalCapabilitiesTest, UnknownTerminalFallsBackToUnicode) {
  FakeEnvironment env;
  env.set("TERM", "dumb");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect();

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kUnicode);
  EXPECT_EQ(caps.detected_terminal, "dumb");
  const auto available = available_graphics_protocols(caps);
  ASSERT_EQ(available.size(), 1u);
  EXPECT_EQ(available[0], GraphicsProtocol::kUnicode);
}

TEST(TerminalCapabilitiesTest, InvalidOverrideIgnoredKeepsDetection) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "iTerm.app");
  env.set("ITERM_SESSION_ID", "w0t0:0");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect("not-a-protocol");

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kITerm2);
  EXPECT_TRUE(caps.override_source.empty());
}

TEST(TerminalCapabilitiesTest, AutoGraphicsFlagKeepsDetection) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "kitty");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect("auto");

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kKitty);
  EXPECT_TRUE(caps.override_source.empty());
}
