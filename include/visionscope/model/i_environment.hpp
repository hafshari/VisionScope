#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace visionscope::model {

class IEnvironment {
 public:
  virtual ~IEnvironment() = default;
  [[nodiscard]] virtual std::optional<std::string> get(
      std::string_view name) const = 0;
};

}  // namespace visionscope::model
