#pragma once

#include "visionscope/model/i_camera_source.hpp"

#include <memory>
#include <string>

namespace visionscope::model {

class SdlCameraSource final : public ICameraSource {
 public:
  SdlCameraSource();
  ~SdlCameraSource() override;

  SdlCameraSource(const SdlCameraSource&) = delete;
  SdlCameraSource& operator=(const SdlCameraSource&) = delete;

  [[nodiscard]] std::vector<CameraDevice> list_devices() override;
  [[nodiscard]] bool open(const std::string& device_id) override;
  void close() override;
  [[nodiscard]] bool is_open() const override;
  [[nodiscard]] std::optional<Frame> grab_frame() override;
  [[nodiscard]] std::string last_error() const override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace visionscope::model
