#include "fakes/fake_camera_source.hpp"

#include "visionscope/viewmodel/camera_view_model.hpp"

#include <gtest/gtest.h>

#include <string>

using visionscope::model::Frame;
using visionscope::model::GraphicsProtocol;
using visionscope::model::TerminalCapabilities;
using visionscope::test::FakeCameraSource;
using visionscope::viewmodel::CameraViewModel;

namespace {

TerminalCapabilities UnicodeCaps() {
  TerminalCapabilities caps;
  caps.preferred = GraphicsProtocol::kUnicode;
  caps.detected_terminal = "test";
  return caps;
}

Frame RgbFrame(int w, int h) {
  Frame frame;
  frame.width = w;
  frame.height = h;
  frame.pixels.assign(static_cast<std::size_t>(w * h * 3), 0);
  return frame;
}

}  // namespace

TEST(CameraViewModelTest, RefreshListsDevicesFromInjectedCamera) {
  FakeCameraSource camera;
  camera.devices = {{"1", "Front"}, {"2", "Back"}};
  const auto caps = UnicodeCaps();

  CameraViewModel vm{camera, caps};

  EXPECT_EQ(vm.devices().size(), 2u);
  EXPECT_EQ(vm.selected_index(), 0);
  EXPECT_NE(vm.status().find("Found 2"), std::string::npos);
}

TEST(CameraViewModelTest, StartFailsWhenNoCameras) {
  FakeCameraSource camera;
  const auto caps = UnicodeCaps();
  CameraViewModel vm{camera, caps};

  EXPECT_FALSE(vm.start());
  EXPECT_FALSE(vm.running());
  EXPECT_NE(vm.status().find("no cameras"), std::string::npos);
}

TEST(CameraViewModelTest, StartAndTickPublishLatestFrame) {
  FakeCameraSource camera;
  camera.devices = {{"9", "Cam"}};
  camera.next_frame = RgbFrame(4, 2);
  const auto caps = UnicodeCaps();

  CameraViewModel vm{camera, caps};
  ASSERT_TRUE(vm.start());
  EXPECT_TRUE(vm.running());
  EXPECT_EQ(camera.open_calls, 1);

  vm.tick();
  const auto latest = vm.latest_frame();
  EXPECT_FALSE(latest.empty());
  EXPECT_EQ(latest.width, 4);
  EXPECT_EQ(latest.height, 2);
  EXPECT_EQ(camera.grab_calls, 1);
}

TEST(CameraViewModelTest, StopClosesCameraAndClearsFrame) {
  FakeCameraSource camera;
  camera.devices = {{"9", "Cam"}};
  camera.next_frame = RgbFrame(2, 2);
  const auto caps = UnicodeCaps();

  CameraViewModel vm{camera, caps};
  ASSERT_TRUE(vm.start());
  vm.tick();
  vm.stop();

  EXPECT_FALSE(vm.running());
  EXPECT_EQ(camera.close_calls, 1);
  EXPECT_TRUE(vm.latest_frame().empty());
}

TEST(CameraViewModelTest, OpenFailureSurfacesStatus) {
  FakeCameraSource camera;
  camera.devices = {{"9", "Cam"}};
  camera.open_should_fail = true;
  const auto caps = UnicodeCaps();

  CameraViewModel vm{camera, caps};
  EXPECT_FALSE(vm.start());
  EXPECT_NE(vm.status().find("Open failed"), std::string::npos);
}
