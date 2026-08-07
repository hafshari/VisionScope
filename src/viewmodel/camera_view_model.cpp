#include "visionscope/viewmodel/camera_view_model.hpp"

namespace visionscope::viewmodel {

CameraViewModel::CameraViewModel(model::ICameraSource& camera,
                                 const model::TerminalCapabilities& caps)
    : camera_(camera), caps_(caps) {
  status_ = "Ready";
  refresh_devices();
}

void CameraViewModel::refresh_devices() {
  devices_ = camera_.list_devices();
  if (devices_.empty()) {
    selected_index_ = 0;
    status_ = "No cameras found";
    if (!camera_.last_error().empty()) {
      status_ += " (" + camera_.last_error() + ")";
    }
    return;
  }
  if (selected_index_ < 0 ||
      selected_index_ >= static_cast<int>(devices_.size())) {
    selected_index_ = 0;
  }
  status_ = "Found " + std::to_string(devices_.size()) + " camera(s)";
}

const std::vector<model::CameraDevice>& CameraViewModel::devices() const {
  return devices_;
}

int CameraViewModel::selected_index() const {
  return selected_index_;
}

void CameraViewModel::select_index(int index) {
  if (index < 0 || index >= static_cast<int>(devices_.size())) {
    return;
  }
  if (running_) {
    stop();
  }
  selected_index_ = index;
  status_ = "Selected " + devices_[static_cast<std::size_t>(index)].name;
}

bool CameraViewModel::start() {
  if (devices_.empty()) {
    status_ = "Cannot start: no cameras";
    return false;
  }
  const auto& device = devices_[static_cast<std::size_t>(selected_index_)];
  if (!camera_.open(device.id)) {
    status_ = "Open failed: " + camera_.last_error();
    running_ = false;
    return false;
  }
  running_ = true;
  status_ = "Streaming " + device.name;
  return true;
}

void CameraViewModel::stop() {
  running_ = false;
  camera_.close();
  latest_frame_ = model::Frame{};
  status_ = "Stopped";
}

bool CameraViewModel::running() const {
  return running_;
}

void CameraViewModel::tick() {
  if (!running_) {
    return;
  }
  auto frame = camera_.grab_frame();
  if (!frame.has_value()) {
    return;
  }
  latest_frame_ = std::move(*frame);
}

model::Frame CameraViewModel::latest_frame() const {
  return latest_frame_;
}

const std::string& CameraViewModel::status() const {
  return status_;
}

const model::TerminalCapabilities& CameraViewModel::capabilities() const {
  return caps_;
}

std::string CameraViewModel::preferred_protocol_label() const {
  return model::graphics_protocol_name(caps_.preferred);
}

}  // namespace visionscope::viewmodel
