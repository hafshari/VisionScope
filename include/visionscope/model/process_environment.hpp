#pragma once

#include "visionscope/model/i_environment.hpp"

namespace visionscope::model {

class ProcessEnvironment final : public IEnvironment {
 public:
  [[nodiscard]] std::optional<std::string> get(
      std::string_view name) const override;
};

}  // namespace visionscope::model
