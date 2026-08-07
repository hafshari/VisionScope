#pragma once

#include "visionscope/model/frame.hpp"

#include <optional>
#include <string>
#include <vector>

namespace visionscope::model {

struct CameraDevice {
  std::string id;
  std::string name;
};

class ICameraSource {
 public:
  virtual ~ICameraSource() = default;

  [[nodiscard]] virtual std::vector<CameraDevice> list_devices() = 0;
  [[nodiscard]] virtual bool open(const std::string& device_id) = 0;
  virtual void close() = 0;
  [[nodiscard]] virtual bool is_open() const = 0;
  [[nodiscard]] virtual std::optional<Frame> grab_frame() = 0;
  [[nodiscard]] virtual std::string last_error() const = 0;
};

}  // namespace visionscope::model
