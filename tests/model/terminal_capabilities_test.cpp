#include "fakes/fake_environment.hpp"

#include "visionscope/model/terminal_capabilities.hpp"

#include <gtest/gtest.h>

using visionscope::model::GraphicsProtocol;
using visionscope::model::TerminalCapabilityDetector;
using visionscope::test::FakeEnvironment;

TEST(TerminalCapabilitiesTest, CliGraphicsOverrideWinsAndIsLabeledCli) {
  FakeEnvironment env;
  env.set("TERM_PROGRAM", "kitty");
  env.set("VISIONSCOPE_GRAPHICS", "sixel");

  TerminalCapabilityDetector detector{env};
  const auto caps = detector.detect("iterm2");

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kITerm2);
  EXPECT_TRUE(caps.iterm2);
  EXPECT_FALSE(caps.kitty);
  EXPECT_EQ(caps.override_source, "cli");
  EXPECT_EQ(caps.detected_terminal, "override");
}

TEST(TerminalCapabilitiesTest, EnvGraphicsOverrideWhenNoCliFlag) {
  FakeEnvironment env;
  env.set("VISIONSCOPE_GRAPHICS", "kitty");

  const auto caps =
      visionscope::model::detect_terminal_capabilities(env, nullptr);

  EXPECT_EQ(caps.preferred, GraphicsProtocol::kKitty);
  EXPECT_EQ(caps.override_source, "env");
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
}
