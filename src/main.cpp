#include "visionscope/model/process_environment.hpp"
#include "visionscope/model/sdl_camera_source.hpp"
#include "visionscope/model/terminal_capabilities.hpp"
#include "visionscope/view/app.hpp"
#include "visionscope/view/make_frame_accelerator.hpp"
#include "visionscope/viewmodel/camera_view_model.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

const char* parse_graphics_flag(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--graphics" && i + 1 < argc) {
      return argv[i + 1];
    }
    if (arg.rfind("--graphics=", 0) == 0) {
      return argv[i] + std::strlen("--graphics=");
    }
    if (arg == "--help" || arg == "-h") {
      std::cout
          << "VisionScope — terminal webcam preview\n"
          << "Usage: visionscope [--graphics=auto|kitty|iterm2|sixel|unicode]\n"
          << "  --graphics=auto   detect from terminal (default)\n"
          << "  --graphics=...    force startup protocol; TUI can still switch\n";
      std::exit(0);
    }
  }
  return nullptr;
}

}  // namespace

int main(int argc, char** argv) {
  // Composition root: owns concrete dependencies; injects by reference.
  const char* graphics_override = parse_graphics_flag(argc, argv);
  visionscope::model::ProcessEnvironment env;
  const auto caps = visionscope::model::detect_terminal_capabilities(
      env, graphics_override);

  visionscope::model::SdlCameraSource camera;
  visionscope::viewmodel::CameraViewModel view_model{camera, caps};
  // Platform Strategy owned here; View borrows by reference (ADR-0005).
  auto frame_accel = visionscope::view::make_frame_accelerator();
  return visionscope::view::run_app(view_model, *frame_accel);
}
