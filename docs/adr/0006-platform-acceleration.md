# ADR-0006: Platform acceleration Strategy and folder layout

- **Status:** Accepted
- **Date:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** solid, strategy, platform, accelerate, imageio

## Context

Live preview needs fast downscale/encode. Apple provides Accelerate + ImageIO; other OSes will get different backends later. Baking `#ifdef __APPLE__` into shared view code couples every protocol builder to one platform and violates Open/Closed and Dependency Inversion ([ADR-0005](0005-tdd-solid-di.md)).

## Decision

**Accepted:** Treat scale/encode as a **Strategy** behind `IFrameAccelerator`.

```text
include/visionscope/view/
  i_frame_accelerator.hpp          # port
  software_frame_accelerator.hpp   # portable default
  make_frame_accelerator.hpp       # composition-root factory

src/view/accel/
  software_frame_accelerator.cpp
  make_frame_accelerator.cpp       # selects backend via CMake compile defs

src/view/platform/
  apple/
    CMakeLists.txt                 # add_subdirectory only when APPLE
    apple_frame_accelerator.hpp
    apple_frame_accelerator.cpp    # Accelerate + ImageIO
  # linux/CMakeLists.txt … (future)
```

- Root `CMakeLists.txt` keeps **portable** sources on `visionscope_core`, then:

```cmake
if(APPLE)
  add_subdirectory(src/view/platform/apple)
endif()
```

- The Apple `CMakeLists.txt` owns: `target_sources`, private include dir, `VISIONSCOPE_HAS_APPLE_FRAME_ACCEL`, and framework link lines.
- Factory uses `#if defined(VISIONSCOPE_HAS_APPLE_FRAME_ACCEL)` (CMake-gated), not raw `#ifdef __APPLE__` scattered in shared files.

## SOLID mapping

| Principle | How this change applies |
|-----------|-------------------------|
| **S** | Apple codecs stay in `platform/apple`; PNG/zlib stay in `protocol_sequences` |
| **O** | New OS backend = new folder + factory branch; protocol code unchanged |
| **L** | Software and Apple both honor `IFrameAccelerator` (empty `encode_iterm_image` → PNG fallback) |
| **I** | Narrow accel port (scale + optional still encode + name) |
| **D** | View depends on `IFrameAccelerator`, not Accelerate/ImageIO headers |

## Consequences

- Positive: testable with `SoftwareFrameAccelerator`; macOS specifics isolated; clear place for Linux VA-API/etc.
- Negative: one extra type + factory vs free functions
- Follow-up: optional `platform/linux/` backend; protocol Strategy objects if bitmap emitters grow further

## Related decisions

- [ADR-0002](0002-terminal-graphics.md) — Graphics adapter in View
- [ADR-0003](0003-conan-cmake-layout.md) — `src/view/` layout
- [ADR-0005](0005-tdd-solid-di.md) — SOLID + reference DI
