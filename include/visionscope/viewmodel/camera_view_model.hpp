#pragma once

#include "visionscope/model/frame.hpp"
#include "visionscope/model/i_camera_source.hpp"
#include "visionscope/model/terminal_capabilities.hpp"

#include <string>
#include <vector>

namespace visionscope::viewmodel {

class CameraViewModel {
 public:
  CameraViewModel(model::ICameraSource& camera,
                  const model::TerminalCapabilities& caps);

  void refresh_devices();
  [[nodiscard]] const std::vector<model::CameraDevice>& devices() const;
  [[nodiscard]] int selected_index() const;
  void select_index(int index);

  [[nodiscard]] bool start();
  void stop();
  [[nodiscard]] bool running() const;

  void tick();

  [[nodiscard]] model::Frame latest_frame() const;
  [[nodiscard]] const std::string& status() const;
  [[nodiscard]] const model::TerminalCapabilities& capabilities() const;
  [[nodiscard]] std::string preferred_protocol_label() const;

 private:
  model::ICameraSource& camera_;
  model::TerminalCapabilities caps_;
  std::vector<model::CameraDevice> devices_;
  int selected_index_ = 0;
  bool running_ = false;
  std::string status_;
  model::Frame latest_frame_;
};

}  // namespace visionscope::viewmodel
