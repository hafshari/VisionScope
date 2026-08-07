#pragma once

#include "visionscope/model/i_environment.hpp"

#include <string>

namespace visionscope::model {

enum class GraphicsProtocol {
  kNone,
  kUnicode,
  kSixel,
  kITerm2,
  kKitty,
};

struct TerminalCapabilities {
  bool kitty = false;
  bool iterm2 = false;
  bool sixel = false;
  GraphicsProtocol preferred = GraphicsProtocol::kUnicode;
  std::string detected_terminal;
  std::string override_source;
};

[[nodiscard]] const char* graphics_protocol_name(GraphicsProtocol protocol);

class TerminalCapabilityDetector {
 public:
  explicit TerminalCapabilityDetector(const IEnvironment& env);

  [[nodiscard]] TerminalCapabilities detect(
      const char* graphics_override = nullptr) const;

 private:
  const IEnvironment& env_;
};

[[nodiscard]] TerminalCapabilities detect_terminal_capabilities(
    const IEnvironment& env, const char* graphics_override = nullptr);

}  // namespace visionscope::model
