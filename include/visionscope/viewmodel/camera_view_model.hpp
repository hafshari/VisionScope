#pragma once

#include "visionscope/model/frame.hpp"
#include "visionscope/model/i_camera_source.hpp"
#include "visionscope/model/terminal_capabilities.hpp"

#include <chrono>
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

  [[nodiscard]] const model::Frame& latest_frame() const;
  [[nodiscard]] double fps() const;
  [[nodiscard]] std::string stats_label() const;
  [[nodiscard]] const std::string& status() const;
  [[nodiscard]] const model::TerminalCapabilities& capabilities() const;

  [[nodiscard]] const std::vector<model::GraphicsProtocol>& available_protocols()
      const;
  [[nodiscard]] model::GraphicsProtocol active_protocol() const;
  [[nodiscard]] bool select_protocol(model::GraphicsProtocol protocol);
  void cycle_protocol();
  [[nodiscard]] std::string active_protocol_label() const;
  [[nodiscard]] std::string preferred_protocol_label() const;

 private:
  void note_frame_captured();
  void reset_stream_stats();

  model::ICameraSource& camera_;
  model::TerminalCapabilities caps_;
  std::vector<model::GraphicsProtocol> available_protocols_;
  model::GraphicsProtocol active_protocol_ = model::GraphicsProtocol::kUnicode;
  std::vector<model::CameraDevice> devices_;
  int selected_index_ = 0;
  bool running_ = false;
  std::string status_;
  model::Frame latest_frame_;
  double fps_ = 0.0;
  int frames_in_window_ = 0;
  std::chrono::steady_clock::time_point fps_window_start_{};
};

}  // namespace visionscope::viewmodel
