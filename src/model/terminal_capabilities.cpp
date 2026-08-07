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
  if (value.empty() || equals_ci(value, "auto") || equals_ci(value, "detect")) {
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

void mark_protocol_available(TerminalCapabilities& caps, GraphicsProtocol p) {
  switch (p) {
    case GraphicsProtocol::kKitty:
      caps.kitty = true;
      break;
    case GraphicsProtocol::kITerm2:
      caps.iterm2 = true;
      break;
    case GraphicsProtocol::kSixel:
      caps.sixel = true;
      break;
    case GraphicsProtocol::kUnicode:
    case GraphicsProtocol::kNone:
      break;
  }
}

GraphicsProtocol prefer_from_flags(const TerminalCapabilities& caps) {
  if (caps.kitty) {
    return GraphicsProtocol::kKitty;
  }
  if (caps.iterm2) {
    return GraphicsProtocol::kITerm2;
  }
  if (caps.sixel) {
    return GraphicsProtocol::kSixel;
  }
  return GraphicsProtocol::kUnicode;
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

std::vector<GraphicsProtocol> available_graphics_protocols(
    const TerminalCapabilities& caps) {
  std::vector<GraphicsProtocol> out;
  out.push_back(GraphicsProtocol::kUnicode);
  if (caps.sixel) {
    out.push_back(GraphicsProtocol::kSixel);
  }
  if (caps.iterm2) {
    out.push_back(GraphicsProtocol::kITerm2);
  }
  if (caps.kitty) {
    out.push_back(GraphicsProtocol::kKitty);
  }
  return out;
}

TerminalCapabilityDetector::TerminalCapabilityDetector(const IEnvironment& env)
    : env_(env) {}

TerminalCapabilities TerminalCapabilityDetector::detect(
    const char* graphics_override) const {
  TerminalCapabilities caps;

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

  caps.preferred = prefer_from_flags(caps);

  if (graphics_override != nullptr && graphics_override[0] != '\0') {
    const GraphicsProtocol forced = parse_override(graphics_override);
    if (forced != GraphicsProtocol::kNone) {
      caps.override_source = "cli";
      caps.preferred = forced;
      mark_protocol_available(caps, forced);
    }
  }

  return caps;
}

TerminalCapabilities detect_terminal_capabilities(const IEnvironment& env,
                                                  const char* graphics_override) {
  return TerminalCapabilityDetector{env}.detect(graphics_override);
}

}  // namespace visionscope::model
