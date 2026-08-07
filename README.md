# VisionScope

TUI for live webcam/camera view in the terminal.

## Status

Live preview MVP: FTXUI chrome, SDL3 capture, protocol auto-detect + runtime switch, Kitty / iTerm2 / Unicode half-block drawing. Sixel is detected but not drawn yet. macOS uses Accelerate + ImageIO for scale/encode ([ADR-0006](docs/adr/0006-platform-acceleration.md)).

## Stack

- **C++20**
- **Conan 2** + **CMake** + **Ninja** ([ADR-0003](docs/adr/0003-conan-cmake-layout.md))
- **FTXUI** for the terminal UI
- **SDL3 Camera** for capture ([ADR-0001](docs/adr/0001-cross-platform-camera-capture.md))
- Terminal graphics: Kitty → iTerm2 → Sixel → Unicode ([ADR-0002](docs/adr/0002-terminal-graphics.md))
- **MVVM** ([ADR-0004](docs/adr/0004-mvvm-structure.md))
- **TDD + SOLID** with Boost.DI-style **reference injection** ([ADR-0005](docs/adr/0005-tdd-solid-di.md))
- Frame scale/encode via `IFrameAccelerator` Strategy; Apple pack under `src/view/platform/apple/` ([ADR-0006](docs/adr/0006-platform-acceleration.md))

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
./build/build/Release/visionscope --graphics=kitty
./build/build/Release/visionscope --graphics=iterm2
./build/build/Release/visionscope --graphics=auto   # same as omitting the flag
```

Protocol selection: auto-detect from the terminal by default, optional `--graphics=` for the startup choice, then switch at runtime in the TUI (menu / `g`). No app-specific env override.

On macOS, grant Terminal/iTerm/WezTerm camera permission when prompted. The status line shows capture rate (`cap`), resolution, protocol, and accel backend (`Accelerate+ImageIO` or `software`).

## Layout

```text
include/visionscope/     headers (model / viewmodel / view)
src/
  main.cpp               composition root
  model/ viewmodel/ view/
  view/accel/            portable IFrameAccelerator + factory
  view/platform/apple/   CMake pack (add_subdirectory on APPLE only)
tests/                   gtest + fakes (reference-injected)
docs/adr/                architecture decisions
recipes/                 reserved for local Conan recipes (e.g. ccap plan B)
```

Constructors take `T&` / `const T&` (or values). Ownership stays in `main` / test fixtures — avoid `unique_ptr`/`shared_ptr` as DI parameters.
