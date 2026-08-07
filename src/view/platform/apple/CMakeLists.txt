# Apple platform pack — only pulled in via root `if(APPLE) add_subdirectory(...)`.
# Owns Accelerate / ImageIO frame acceleration for the View layer.

target_sources(visionscope_core PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/apple_frame_accelerator.cpp
)

# Headers in this directory are visible to visionscope_core TUs (e.g. factory).
target_include_directories(visionscope_core PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(visionscope_core PUBLIC
  VISIONSCOPE_HAS_APPLE_FRAME_ACCEL=1
)

target_link_libraries(visionscope_core PUBLIC
  "-framework Accelerate"
  "-framework CoreGraphics"
  "-framework ImageIO"
)
