from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps


class VisionScopeConan(ConanFile):
    name = "visionscope"
    version = "0.1.0"
    package_type = "application"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_tests": [True, False],
    }
    default_options = {
        "with_tests": True,
    }

    def requirements(self):
        self.requires("ftxui/6.1.9")
        self.requires("sdl/3.4.8")

    def build_requirements(self):
        if self.options.with_tests:
            self.test_requires("gtest/1.16.0")

    def configure(self):
        self.options["sdl"].camera = True

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_CXX_STANDARD"] = "20"
        tc.variables["VISIONSCOPE_BUILD_TESTS"] = bool(self.options.with_tests)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
