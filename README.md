# VisionScope

TUI for live webcam/camera view in the terminal.

## Status

Scaffold: FTXUI shell, SDL3 camera source, terminal capability detection, MVVM layout. True Kitty/iTerm2/Sixel frame drawing lands in the live-preview MVP.

## Stack

- **C++20**
- **Conan 2** + **CMake** + **Ninja** ([ADR-0003](docs/adr/0003-conan-cmake-layout.md))
- **FTXUI** for the terminal UI
- **SDL3 Camera** for capture ([ADR-0001](docs/adr/0001-cross-platform-camera-capture.md))
- Terminal image-capability detection with protocol priority Kitty → iTerm2 → Sixel → Unicode ([ADR-0002](docs/adr/0002-terminal-graphics.md))
- **MVVM** structure ([ADR-0004](docs/adr/0004-mvvm-structure.md))
- **TDD + SOLID** with Boost.DI-style **reference injection** ([ADR-0005](docs/adr/0005-tdd-solid-di.md))

## Build

```bash
python -m pip install -r requirements.txt
conan install . --output-folder=build --build=missing -s build_type=Release
cmake --preset conan-release
cmake --build --preset conan-release
```

Binary: `build/build/Release/visionscope` (Conan `cmake_layout`).

## Test

```bash
cmake --build --preset conan-release --target visionscope_tests
ctest --test-dir build/build/Release --output-on-failure
# or:
./build/build/Release/visionscope_tests
```

New Model/ViewModel behavior: write a failing gtest first, then implement.

## Run

```bash
./build/build/Release/visionscope
./build/build/Release/visionscope --graphics=unicode
VISIONSCOPE_GRAPHICS=iterm2 ./build/build/Release/visionscope
```

On macOS, grant Terminal/iTerm/WezTerm camera permission when prompted.

## Layout

```text
include/visionscope/   headers (model / viewmodel / view)
src/                   implementations + main (composition root)
tests/                 gtest + fakes (reference-injected)
docs/adr/              architecture decisions
recipes/               reserved for local Conan recipes (e.g. ccap plan B)
```

Constructors take `T&` / `const T&` (or values). Ownership stays in `main` / test fixtures — avoid `unique_ptr`/`shared_ptr` as DI parameters.
