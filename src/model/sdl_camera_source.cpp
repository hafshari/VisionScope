#include "visionscope/model/sdl_camera_source.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace visionscope::model {

struct SdlCameraSource::Impl {
  bool camera_subsystem_ok = false;
  SDL_Camera* camera = nullptr;
  std::string last_error;
  std::string open_device_id;
};

SdlCameraSource::SdlCameraSource() : impl_(std::make_unique<Impl>()) {
  if (!SDL_InitSubSystem(SDL_INIT_CAMERA)) {
    impl_->last_error = SDL_GetError();
    return;
  }
  impl_->camera_subsystem_ok = true;
}

SdlCameraSource::~SdlCameraSource() {
  close();
  if (impl_->camera_subsystem_ok) {
    SDL_QuitSubSystem(SDL_INIT_CAMERA);
  }
}

std::vector<CameraDevice> SdlCameraSource::list_devices() {
  std::vector<CameraDevice> devices;
  if (!impl_->camera_subsystem_ok) {
    return devices;
  }

  int count = 0;
  SDL_CameraID* ids = SDL_GetCameras(&count);
  if (ids == nullptr) {
    impl_->last_error = SDL_GetError();
    return devices;
  }

  devices.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    CameraDevice device;
    device.id = std::to_string(ids[i]);
    const char* name = SDL_GetCameraName(ids[i]);
    device.name = name != nullptr ? name : ("Camera " + device.id);
    devices.push_back(std::move(device));
  }

  SDL_free(ids);
  return devices;
}

bool SdlCameraSource::open(const std::string& device_id) {
  close();
  if (!impl_->camera_subsystem_ok) {
    impl_->last_error = "SDL camera subsystem not initialized";
    return false;
  }

  SDL_CameraID target = 0;
  try {
    target = static_cast<SDL_CameraID>(std::stoul(device_id));
  } catch (...) {
    impl_->last_error = "Invalid device id: " + device_id;
    return false;
  }

  impl_->camera = SDL_OpenCamera(target, nullptr);
  if (impl_->camera == nullptr) {
    impl_->last_error = SDL_GetError();
    return false;
  }

  impl_->open_device_id = device_id;
  impl_->last_error.clear();
  return true;
}

void SdlCameraSource::close() {
  if (impl_->camera != nullptr) {
    SDL_CloseCamera(impl_->camera);
    impl_->camera = nullptr;
  }
  impl_->open_device_id.clear();
}

bool SdlCameraSource::is_open() const {
  return impl_->camera != nullptr;
}

std::optional<Frame> SdlCameraSource::grab_frame() {
  if (impl_->camera == nullptr) {
    return std::nullopt;
  }

  Uint64 timestamp_ns = 0;
  SDL_Surface* surface = SDL_AcquireCameraFrame(impl_->camera, &timestamp_ns);
  if (surface == nullptr) {
    return std::nullopt;
  }

  // Scaffold: convert to RGB24 when possible; full format matrix comes with MVP.
  SDL_Surface* converted =
      SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGB24);
  SDL_ReleaseCameraFrame(impl_->camera, surface);
  if (converted == nullptr) {
    impl_->last_error = SDL_GetError();
    return std::nullopt;
  }

  Frame frame;
  frame.width = converted->w;
  frame.height = converted->h;
  frame.format = PixelFormat::kRgb24;
  const int row_bytes = frame.width * 3;
  frame.pixels.resize(static_cast<std::size_t>(row_bytes * frame.height));

  if (SDL_LockSurface(converted)) {
    const auto* src = static_cast<const std::uint8_t*>(converted->pixels);
    for (int y = 0; y < frame.height; ++y) {
      std::memcpy(frame.pixels.data() + static_cast<std::size_t>(y * row_bytes),
                  src + static_cast<std::size_t>(y * converted->pitch),
                  static_cast<std::size_t>(row_bytes));
    }
    SDL_UnlockSurface(converted);
  } else {
    impl_->last_error = SDL_GetError();
    SDL_DestroySurface(converted);
    return std::nullopt;
  }

  SDL_DestroySurface(converted);
  return frame;
}

std::string SdlCameraSource::last_error() const {
  return impl_->last_error;
}

}  // namespace visionscope::model
