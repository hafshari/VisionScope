#include "visionscope/view/make_frame_accelerator.hpp"

#include "visionscope/view/software_frame_accelerator.hpp"

// Set by src/view/platform/apple/CMakeLists.txt when that pack is included.
#if defined(VISIONSCOPE_HAS_APPLE_FRAME_ACCEL)
#include "apple_frame_accelerator.hpp"
#endif

namespace visionscope::view {

std::unique_ptr<IFrameAccelerator> make_frame_accelerator() {
#if defined(VISIONSCOPE_HAS_APPLE_FRAME_ACCEL)
  return std::make_unique<platform::apple::AppleFrameAccelerator>();
#else
  return std::make_unique<SoftwareFrameAccelerator>();
#endif
}

}  // namespace visionscope::view
