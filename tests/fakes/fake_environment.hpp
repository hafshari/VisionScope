#pragma once

#include "visionscope/model/i_environment.hpp"

#include <map>
#include <string>
#include <string_view>

namespace visionscope::test {

class FakeEnvironment final : public model::IEnvironment {
 public:
  void set(std::string name, std::string value) {
    values_[std::move(name)] = std::move(value);
  }

  [[nodiscard]] std::optional<std::string> get(
      std::string_view name) const override {
    const auto it = values_.find(std::string{name});
    if (it == values_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

 private:
  std::map<std::string, std::string> values_;
};

}  // namespace visionscope::test
