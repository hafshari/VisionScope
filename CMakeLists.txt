cmake_minimum_required(VERSION 3.20)
project(VisionScope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VISIONSCOPE_BUILD_TESTS "Build unit tests" ON)

find_package(ftxui REQUIRED)
find_package(SDL3 REQUIRED)

add_library(visionscope_core
  src/model/frame.cpp
  src/model/process_environment.cpp
  src/model/terminal_capabilities.cpp
  src/model/sdl_camera_source.cpp
  src/viewmodel/camera_view_model.cpp
  src/view/terminal_graphics_adapter.cpp
  src/view/app.cpp
)

target_include_directories(visionscope_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(visionscope_core PUBLIC
  ftxui::screen
  ftxui::dom
  ftxui::component
  SDL3::SDL3
)

add_executable(visionscope src/main.cpp)
target_link_libraries(visionscope PRIVATE visionscope_core)
install(TARGETS visionscope RUNTIME DESTINATION bin)

if(VISIONSCOPE_BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)
  add_executable(visionscope_tests
    tests/model/terminal_capabilities_test.cpp
    tests/viewmodel/camera_view_model_test.cpp
    tests/view/terminal_graphics_adapter_test.cpp
  )
  target_include_directories(visionscope_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_link_libraries(visionscope_tests PRIVATE
    visionscope_core
    GTest::gtest_main
  )
  include(GoogleTest)
  gtest_discover_tests(visionscope_tests)
endif()
