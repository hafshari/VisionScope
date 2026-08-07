#pragma once

#include "visionscope/view/i_frame_accelerator.hpp"

#include <memory>

namespace visionscope::view {

// Composition-root helper: picks the best platform backend (Apple → Accelerate
// + ImageIO; else software). Ownership stays with the caller.
[[nodiscard]] std::unique_ptr<IFrameAccelerator> make_frame_accelerator();

}  // namespace visionscope::view
