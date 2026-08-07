#pragma once

#include "visionscope/view/i_frame_accelerator.hpp"
#include "visionscope/viewmodel/camera_view_model.hpp"

namespace visionscope::view {

int run_app(viewmodel::CameraViewModel& view_model, IFrameAccelerator& accel);

}  // namespace visionscope::view
