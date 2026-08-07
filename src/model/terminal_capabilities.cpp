#include "visionscope/model/terminal_capabilities.hpp"

#include <cctype>
#include <string>
#include <string_view>

namespace visionscope::model {
namespace {

bool equals_ci(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i = 0; i < a.size(); ++i) {
    const auto ca = static_cast<unsigned char>(a[i]);
    const auto cb = static_cast<unsigned char>(b[i]);
    if (std::tolower(ca) != std::tolower(cb)) {
      return false;
    }
  }
  return true;
}

GraphicsProtocol parse_override(std::string_view value) {
  if (value.empty()) {
    return GraphicsProtocol::kNone;
  }
  if (equals_ci(value, "kitty")) {
    return GraphicsProtocol::kKitty;
  }
  if (equals_ci(value, "iterm2") || equals_ci(value, "iterm")) {
    return GraphicsProtocol::kITerm2;
  }
  if (equals_ci(value, "sixel")) {
    return GraphicsProtocol::kSixel;
  }
  if (equals_ci(value, "unicode") || equals_ci(value, "none") ||
      equals_ci(value, "block")) {
    return GraphicsProtocol::kUnicode;
  }
  return GraphicsProtocol::kNone;
}

std::string env_or_empty(const IEnvironment& env, std::string_view name) {
  if (auto value = env.get(name)) {
    return *value;
  }
  return {};
}

bool env_nonempty(const IEnvironment& env, std::string_view name) {
  const auto value = env.get(name);
  return value.has_value() && !value->empty();
}

}  // namespace

const char* graphics_protocol_name(GraphicsProtocol protocol) {
  switch (protocol) {
    case GraphicsProtocol::kKitty:
      return "kitty";
    case GraphicsProtocol::kITerm2:
      return "iterm2";
    case GraphicsProtocol::kSixel:
      return "sixel";
    case GraphicsProtocol::kUnicode:
      return "unicode";
    case GraphicsProtocol::kNone:
      return "none";
  }
  return "none";
}

TerminalCapabilityDetector::TerminalCapabilityDetector(const IEnvironment& env)
    : env_(env) {}

TerminalCapabilities TerminalCapabilityDetector::detect(
    const char* graphics_override) const {
  TerminalCapabilities caps;

  std::string override_source;
  std::string override_value;
  if (graphics_override != nullptr && graphics_override[0] != '\0') {
    override_value = graphics_override;
    override_source = "cli";
  } else if (auto from_env = env_.get("VISIONSCOPE_GRAPHICS")) {
    override_value = *from_env;
    if (!override_value.empty()) {
      override_source = "env";
    }
  }

  const GraphicsProtocol forced = parse_override(override_value);
  if (forced != GraphicsProtocol::kNone) {
    caps.override_source = override_source;
    caps.preferred = forced;
    caps.kitty = forced == GraphicsProtocol::kKitty;
    caps.iterm2 = forced == GraphicsProtocol::kITerm2;
    caps.sixel = forced == GraphicsProtocol::kSixel;
    caps.detected_terminal = "override";
    return caps;
  }

  const std::string term_program = env_or_empty(env_, "TERM_PROGRAM");
  const std::string term = env_or_empty(env_, "TERM");

  if (env_nonempty(env_, "KITTY_WINDOW_ID") || equals_ci(term_program, "kitty") ||
      env_nonempty(env_, "GHOSTTY_RESOURCES_DIR") ||
      equals_ci(term_program, "ghostty")) {
    caps.kitty = true;
  }

  if (env_nonempty(env_, "ITERM_SESSION_ID") ||
      equals_ci(term_program, "iTerm.app") || equals_ci(term_program, "iTerm2")) {
    caps.iterm2 = true;
  }

  if (env_nonempty(env_, "WEZTERM_PANE") || equals_ci(term_program, "WezTerm")) {
    caps.kitty = true;
    caps.iterm2 = true;
    caps.sixel = true;
  }

  if (equals_ci(term_program, "vscode")) {
    caps.iterm2 = true;
    caps.sixel = true;
  }

  if (term.find("mlterm") != std::string::npos ||
      term.find("foot") != std::string::npos) {
    caps.sixel = true;
  }

  if (!term_program.empty()) {
    caps.detected_terminal = term_program;
  } else if (!term.empty()) {
    caps.detected_terminal = term;
  } else {
    caps.detected_terminal = "unknown";
  }

  if (caps.kitty) {
    caps.preferred = GraphicsProtocol::kKitty;
  } else if (caps.iterm2) {
    caps.preferred = GraphicsProtocol::kITerm2;
  } else if (caps.sixel) {
    caps.preferred = GraphicsProtocol::kSixel;
  } else {
    caps.preferred = GraphicsProtocol::kUnicode;
  }

  return caps;
}

TerminalCapabilities detect_terminal_capabilities(const IEnvironment& env,
                                                  const char* graphics_override) {
  return TerminalCapabilityDetector{env}.detect(graphics_override);
}

}  // namespace visionscope::model
