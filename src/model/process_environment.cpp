#include "visionscope/model/process_environment.hpp"

#include <cstdlib>
#include <string>

namespace visionscope::model {

std::optional<std::string> ProcessEnvironment::get(std::string_view name) const {
  const std::string key{name};
  const char* value = std::getenv(key.c_str());
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string{value};
}

}  // namespace visionscope::model
