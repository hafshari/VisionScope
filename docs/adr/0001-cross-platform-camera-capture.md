# ADR-0001: Cross-platform camera capture backend

- **Status:** Accepted
- **Date:** 2026-07-19
- **Accepted:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** capture, camera, webcam, cross-platform, conan, tui

## Context

VisionScope is a terminal (TUI) application that shows live webcam/camera feed in the CLI. The intended stack is C/C++, Conan for dependencies, and FTXUI for the UI. The tool must be **cross-platform** (at least macOS, Windows, and Linux).

There is no single OS webcam API. Platform-native capture surfaces differ:

| OS | Native API(s) |
|----|----------------|
| macOS | AVFoundation |
| Windows | Media Foundation, DirectShow |
| Linux | V4L2 (sometimes libcamera) |

VisionScope therefore needs either:

1. A library that abstracts those backends and returns raw frames (RGB/YUV/etc.), or
2. A thin in-house abstraction over platform-native APIs.

The TUI path does **not** require a GUI window from the capture library. Frames are consumed by a terminal graphics adapter (Kitty / iTerm2 / Sixel / Unicode fallback) and rendered through FTXUI (see [ADR-0002](0002-terminal-graphics.md), [ADR-0004](0004-mvvm-structure.md)).

```text
Webcam hardware
      │
      ▼
 Capture backend ──► RGB / YUV frames
      │
      ▼
 Terminal image adapter
      ├── Kitty graphics
      ├── iTerm2 inline images
      ├── Sixel
      └── Block / ASCII fallback
      │
      ▼
    FTXUI
```

## Decision

**Accepted default backend: SDL3 Camera API**, consumed behind an internal `ICameraSource` interface.

Rationale for v1:

- Available on Conan Center as [`sdl`](https://conan.io/center/recipes/sdl) with `camera=True`
- Purpose-built frame API and first-class permission handling
- Medium binary weight vs OpenCV; no custom packaging vs ccap
- Matches a preview-only MVP; recording/RTSP remain out of scope for v1

`ICameraSource` remains the only type the ViewModel/UI depend on, so OpenCV, FFmpeg, or ccap can be added later without rewriting FTXUI.

### Decision criteria

| Criterion | Why it matters for VisionScope |
|-----------|--------------------------------|
| Cross-platform desktop (macOS / Windows / Linux) | Stated project requirement |
| C/C++ API | Project language |
| Conan / easy packaging | Planned dependency management |
| Frame access (not GUI-centric) | TUI-only display path |
| Device enumerate + open + close | Multi-camera UX |
| Permission handling | Required on macOS (and increasingly elsewhere) |
| Binary / build weight | CLI tool; prefer lean deps when possible |
| License compatibility | Public MIT repo |
| Extensibility | Swap backends without rewriting FTXUI |

## Options considered

### 1. SDL3 Camera API — **Accepted (default)**

**What it is:** First-class webcam capture in SDL 3 (`SDL_GetCameras`, `SDL_OpenCamera`, `SDL_AcquireCameraFrame` / `SDL_ReleaseCameraFrame`). Frames arrive as `SDL_Surface` pixel buffers.

**Platforms:** Windows, macOS, Linux (also Android / iOS / web — out of current scope).

**Packaging:** Conan Center `sdl` (e.g. 3.4.8); packages expose `camera=True`.

**License:** zlib

**Strengths:**

- Purpose-built for reading cameras as frames
- Relatively light compared with OpenCV / GStreamer
- Explicit permission model (`SDL_GetCameraPermissionState`, approve/deny events)
- Fits TUI: no SDL window required just to obtain frames (camera subsystem + event handling still needed)
- Well-maintained cross-platform project; Conan Center binaries already ship with camera enabled

**Weaknesses / risks:**

- Camera API is relatively new (SDL 3.x)
- Consumer cameras often emit black/underexposed frames during warm-up; callers should drop early frames
- App must init SDL camera subsystem and handle permission events
- Pulls broader SDL surface even when only camera is used

**References:**

- https://wiki.libsdl.org/SDL3/CategoryCamera
- https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_camera.h
- https://conan.io/center/recipes/sdl

---

### 2. OpenCV (`cv::VideoCapture`) — deferred plan B

**What it is:** Mature computer-vision library whose `videoio` module opens webcams via platform backends.

**Packaging:** Conan Center `opencv` (e.g. 4.14.0). Enable `videoio`; slim unused modules.

**License:** Apache-2.0

**Strengths:** Excellent device coverage; trivial path to filters/CV later.

**Weaknesses:** Heavy build/binary; Conan option surface; permissions less first-class than SDL.

**Fit:** Revisit if CV features become central soon.

---

### 3. FFmpeg (`libavdevice`) — deferred

**Fit:** Strong if capture + encode/record becomes primary; overkill for preview-only MVP.

**Packaging:** Conan Center `ffmpeg`.

---

### 4. GStreamer — deferred / rejected for MVP

**Fit:** Only if pipelines (RTSP / multi-source) become core. Too heavy for focused TUI preview.

---

### 5. ccap (CameraCapture) — deferred plan B (lean footprint)

**What it is:** Lightweight dedicated C/C++ camera capture library with hardware-accelerated pixel format conversion. Desktop backends: Windows (DirectShow default + Media Foundation), macOS/iOS (AVFoundation), Linux (V4L2).

**Packaging:** **Not on Conan Center.** Upstream documents CMake / `FetchContent` / Homebrew. A local CCI-style recipe under `recipes/ccap/` (then optional upstream PR) is the path if we switch; see [Upstreaming to Conan Center](../conan/upstreaming-to-conan-center.md).

**License:** MIT

**Strengths:** Lightest capture-only dep; simple frame API; virtual-cam friendly on Windows; video file playback via same API.

**Weaknesses:** Smaller community; VisionScope would own packaging/CI until CCI merge.

**Fit:** Attractive if binary size is prioritized and maintainers accept packaging ownership.

**References:**

- https://github.com/wysaid/CameraCapture
- https://ccap.work/

---

### 6. libuvc — rejected as sole backend

Does not cover many built-in laptop cameras (especially macOS FaceTime / non-UVC). Possible optional backend later for UVC-only devices.

---

### 7. Homegrown platform backends — rejected for initial phase

High maintenance across three OS APIs; duplicates work already done by SDL / OpenCV / FFmpeg / ccap.

---

## Comparison summary

| Option | Cross-platform | Conan Center | Weight | License | Best when… | Status |
|--------|----------------|--------------|--------|---------|------------|--------|
| **SDL3 Camera** | Excellent | Yes (`sdl`, `camera=True`) | Light–medium | zlib | Frames + permissions, Conan-first | **Accepted default** |
| **OpenCV** | Excellent | Yes (`opencv`) | Heavy | Apache-2.0 | Webcam reliability + future CV | Plan B |
| **FFmpeg** | Excellent | Yes (`ffmpeg`) | Medium–heavy | LGPL/GPL mix | Capture + record/stream | Deferred |
| **GStreamer** | Excellent | Yes (`gstreamer`) | Heavy | LGPL | Pipelines / RTSP | Deferred |
| **ccap** | Good (desktop) | No (local recipe feasible) | Lightest dedicated | MIT | Minimal capture-only dep | Plan B |
| **libuvc** | Partial | Possible | Light | BSD-style | UVC USB only | Rejected as sole |
| **DIY native** | Full (DIY) | N/A | Code-heavy | N/A | Absolute control | Rejected early |

## Consequences

### Accepted (SDL3)

- Positive: lean Conan dependency; first-class permissions; simple frame loop for TUI adapters
- Negative: newer camera API; may need frame warm-up / format conversion helpers
- Follow-up: implement `ICameraSource` with an SDL3-backed class; keep OpenCV/ccap as drop-in alternatives behind the same interface

### Regardless of backend

- Keep FTXUI and terminal graphics adapters independent of the capture library ([ADR-0002](0002-terminal-graphics.md), [ADR-0004](0004-mvvm-structure.md))
- Document OS permission requirements (especially macOS camera entitlement / Info.plist when distributing .app bundles later)
- Prefer delivering frames in a stable internal pixel format (RGB24 or RGBA) at the adapter boundary

## Resolved product questions (v1)

1. MVP is **preview-only** (no recording in v1).
2. **Network cameras / RTSP** are out of scope for v1.
3. Prefer **Conan-Center-first** packaging for v1 (favor SDL3 over owning a ccap recipe immediately).
4. Custom **ccap** Conan recipe remains a documented plan-B packaging path, not required for the default stack.

## Related decisions

- [ADR-0002](0002-terminal-graphics.md) — Terminal image capability detection / graphics protocol selection
- [ADR-0003](0003-conan-cmake-layout.md) — Conan + CMake project layout
- [ADR-0004](0004-mvvm-structure.md) — MVVM application structure
