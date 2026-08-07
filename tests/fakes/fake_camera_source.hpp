#pragma once

#include "visionscope/model/i_camera_source.hpp"

#include <utility>

namespace visionscope::test {

class FakeCameraSource final : public model::ICameraSource {
 public:
  std::vector<model::CameraDevice> devices;
  std::string error;
  bool open_should_fail = false;
  model::Frame next_frame;
  int open_calls = 0;
  int close_calls = 0;
  int grab_calls = 0;
  bool opened = false;

  [[nodiscard]] std::vector<model::CameraDevice> list_devices() override {
    return devices;
  }

  [[nodiscard]] bool open(const std::string& /*device_id*/) override {
    ++open_calls;
    if (open_should_fail) {
      error = "open failed";
      opened = false;
      return false;
    }
    opened = true;
    error.clear();
    return true;
  }

  void close() override {
    ++close_calls;
    opened = false;
  }

  [[nodiscard]] bool is_open() const override {
    return opened;
  }

  [[nodiscard]] std::optional<model::Frame> grab_frame() override {
    ++grab_calls;
    if (!opened || next_frame.empty()) {
      return std::nullopt;
    }
    return next_frame;
  }

  [[nodiscard]] std::string last_error() const override {
    return error;
  }
};

}  // namespace visionscope::test
