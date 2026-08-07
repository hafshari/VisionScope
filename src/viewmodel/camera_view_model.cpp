#include "visionscope/viewmodel/camera_view_model.hpp"

#include <algorithm>
#include <sstream>

namespace visionscope::viewmodel {

CameraViewModel::CameraViewModel(model::ICameraSource& camera,
                                 const model::TerminalCapabilities& caps)
    : camera_(camera),
      caps_(caps),
      available_protocols_(model::available_graphics_protocols(caps)),
      active_protocol_(caps.preferred) {
  status_ = "Ready";
  if (std::find(available_protocols_.begin(), available_protocols_.end(),
                 active_protocol_) == available_protocols_.end()) {
    active_protocol_ = model::GraphicsProtocol::kUnicode;
  }
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
  reset_stream_stats();
  status_ = "Streaming " + device.name;
  return true;
}

void CameraViewModel::stop() {
  running_ = false;
  camera_.close();
  latest_frame_ = model::Frame{};
  reset_stream_stats();
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
  note_frame_captured();
}

void CameraViewModel::note_frame_captured() {
  const auto now = std::chrono::steady_clock::now();
  if (frames_in_window_ == 0) {
    fps_window_start_ = now;
  }
  ++frames_in_window_;

  const double elapsed =
      std::chrono::duration<double>(now - fps_window_start_).count();
  if (elapsed >= 0.5) {
    fps_ = static_cast<double>(frames_in_window_) / elapsed;
    frames_in_window_ = 0;
    fps_window_start_ = now;
  }
}

void CameraViewModel::reset_stream_stats() {
  fps_ = 0.0;
  frames_in_window_ = 0;
  fps_window_start_ = {};
}

const model::Frame& CameraViewModel::latest_frame() const {
  return latest_frame_;
}

double CameraViewModel::fps() const {
  return fps_;
}

std::string CameraViewModel::stats_label() const {
  std::ostringstream oss;
  if (latest_frame_.empty()) {
    oss << "res —";
  } else {
    oss << "res " << latest_frame_.width << "x" << latest_frame_.height;
  }
  oss << " · cap ";
  if (fps_ <= 0.0) {
    oss << "—";
  } else {
    oss.setf(std::ios::fixed);
    oss.precision(fps_ >= 10.0 ? 0 : 1);
    oss << fps_;
  }
  return oss.str();
}

const std::string& CameraViewModel::status() const {
  return status_;
}

const model::TerminalCapabilities& CameraViewModel::capabilities() const {
  return caps_;
}

const std::vector<model::GraphicsProtocol>& CameraViewModel::available_protocols()
    const {
  return available_protocols_;
}

model::GraphicsProtocol CameraViewModel::active_protocol() const {
  return active_protocol_;
}

bool CameraViewModel::select_protocol(model::GraphicsProtocol protocol) {
  if (std::find(available_protocols_.begin(), available_protocols_.end(),
                 protocol) == available_protocols_.end()) {
    return false;
  }
  active_protocol_ = protocol;
  status_ = std::string("Graphics: ") + model::graphics_protocol_name(protocol);
  return true;
}

void CameraViewModel::cycle_protocol() {
  if (available_protocols_.empty()) {
    return;
  }
  auto it = std::find(available_protocols_.begin(), available_protocols_.end(),
                      active_protocol_);
  std::size_t index = 0;
  if (it != available_protocols_.end()) {
    index = static_cast<std::size_t>(it - available_protocols_.begin());
    index = (index + 1) % available_protocols_.size();
  }
  (void)select_protocol(available_protocols_[index]);
}

std::string CameraViewModel::active_protocol_label() const {
  return model::graphics_protocol_name(active_protocol_);
}

std::string CameraViewModel::preferred_protocol_label() const {
  return model::graphics_protocol_name(caps_.preferred);
}

}  // namespace visionscope::viewmodel
