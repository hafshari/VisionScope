# ADR-0001: Cross-platform camera capture backend

- **Status:** Proposed
- **Date:** 2026-07-19
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

The TUI path does **not** require a GUI window from the capture library. Frames are consumed by a terminal graphics adapter (Kitty / iTerm2 / Sixel / Unicode fallback) and rendered through FTXUI.

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

This ADR records the investigation of capture options so a later decision can be Accepted with full context.

## Decision

**Proposed (not yet accepted):** Keep the capture layer behind a small `ICameraSource`-style interface, and evaluate concrete backends against the criteria below before locking one as default.

No backend is Accepted in this ADR. The leading candidate for a first implementation is **SDL3 Camera API**, with OpenCV and FFmpeg as strong plan-B options depending on product goals.

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

### 1. SDL3 Camera API

**What it is:** First-class webcam capture in SDL 3 (`SDL_GetCameras`, `SDL_OpenCamera`, `SDL_AcquireCameraFrame` / `SDL_ReleaseCameraFrame`). Frames arrive as `SDL_Surface` pixel buffers.

**Platforms:** Windows, macOS, Linux (also Android / iOS / web — out of current scope but indicates maturity investment).

**Packaging:** Available via Conan Center as `sdl`.

**Strengths:**

- Purpose-built for reading cameras as frames
- Relatively light compared with OpenCV / GStreamer
- Explicit permission model (`SDL_GetCameraPermissionState`, approve/deny events)
- Fits TUI: no SDL window required just to obtain frames (camera subsystem + event handling still needed)
- Well-maintained cross-platform project

**Weaknesses / risks:**

- Camera API is relatively new (SDL 3.x)
- Consumer cameras often emit black/underexposed frames during warm-up; callers should drop early frames
- App must init SDL camera subsystem and handle permission events

**Fit for VisionScope:** Strong default candidate for “preview frames in a terminal.”

**References:**

- https://wiki.libsdl.org/SDL3/CategoryCamera
- https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_camera.h

---

### 2. OpenCV (`cv::VideoCapture`)

**What it is:** Mature computer-vision library whose `videoio` module opens webcams via platform backends (AVFoundation, V4L2, Media Foundation / DirectShow, optionally FFmpeg / GStreamer).

**Packaging:** Conan Center `opencv` (e.g. 4.13.x). Backend-related options in the recipe matter (`videoio`, FFmpeg, experimental GStreamer hooks, etc.).

**Strengths:**

- Extremely common “webcam just works” path
- Simple device index / resolution / FPS controls
- Natural if VisionScope later adds CV features (filters, detection, transforms)

**Weaknesses / risks:**

- Heavy if capture is the only need (build time, binary size, transitive deps)
- Correct Conan options required to enable the desired backends
- Permission UX is more “whatever the backend does” than a first-class SDL-style permissions API

**Fit for VisionScope:** Best plan B if device quirks or CV features become central soon.

**References:**

- https://opencv.org/
- https://conan.io/center/recipes/opencv

---

### 3. FFmpeg (`libavdevice` + decode path)

**What it is:** Media framework; `libavdevice` opens capture devices (`avfoundation`, `dshow` / Media Foundation paths, `v4l2`) and feeds decode/convert pipelines.

**Packaging:** Conan Center `ffmpeg`.

**Strengths:**

- Excellent if recording, remuxing, or streaming become core features
- Very broad format / codec / device coverage
- Same stack used by many media tools

**Weaknesses / risks:**

- Lower-level, more boilerplate for a simple live preview
- Overkill if the product stays “TUI preview only”
- Build options and license combinations need care in a public MIT project

**Fit for VisionScope:** Strong if capture + encode/record becomes a primary goal; otherwise deferred.

**References:**

- https://ffmpeg.org/libavdevice.html
- https://conan.io/center/recipes/ffmpeg

---

### 4. GStreamer

**What it is:** Pipeline-oriented multimedia framework with platform source elements and rich plugin ecosystem (local cams, RTSP, filters, sinks).

**Packaging:** Conan Center `gstreamer` (+ plugin packages / system plugin expectations).

**Strengths:**

- Powerful for complex graphs (RTSP, transforms, multi-source)
- Truly cross-platform via plugins

**Weaknesses / risks:**

- Large runtime and conceptual surface area
- Steep API for a focused TUI preview
- Plugin discovery / packaging friction across OS installers

**Fit for VisionScope:** Only if pipelines (e.g. network cameras / RTSP) become a core product requirement.

**References:**

- https://gstreamer.freedesktop.org/
- https://conan.io/center/recipes/gstreamer

---

### 5. ccap (CameraCapture)

**What it is:** Lightweight dedicated C/C++ camera capture library with hardware-accelerated pixel format conversion. Desktop backends: Windows (DirectShow default + Media Foundation), macOS/iOS (AVFoundation), Linux (V4L2).

**Packaging:** Not on Conan Center at investigation time → would need a custom Conan recipe, CMake `FetchContent`, or vendoring.

**Strengths:**

- Small, capture-focused API (C++ and C)
- Explicitly aimed at “give me frames” use cases
- OpenCV interop examples exist upstream

**Weaknesses / risks:**

- Smaller ecosystem / community than SDL, OpenCV, FFmpeg
- Extra packaging work for a Conan-first repo
- Less battle-tested in diverse CI environments

**Fit for VisionScope:** Attractive if lean footprint is prioritized and maintainers accept packaging ownership.

**References:**

- https://github.com/wysaid/CameraCapture
- https://ccap.work/

---

### 6. libuvc

**What it is:** Userspace USB Video Class (UVC) capture library.

**Strengths:**

- Lightweight for pure UVC USB webcams
- Useful as a specialized backend

**Weaknesses / risks:**

- Does not cover many built-in laptop cameras well (especially macOS FaceTime / non-UVC paths)
- Insufficient as the **sole** cross-platform backend

**Fit for VisionScope:** Rejected as primary backend; possible optional backend later for UVC-only devices.

---

### 7. Homegrown platform backends (AVFoundation + Media Foundation + V4L2)

**What it is:** Maintain three native capture implementations behind an internal interface (OBS-style approach).

**Strengths:**

- Maximum control, minimal unrelated dependencies
- Can tune permission prompts, formats, and device quirks precisely

**Weaknesses / risks:**

- High ongoing maintenance (3 platforms × driver quirks × OS updates)
- Slowest path to a working MVP
- Duplicates work already done by SDL / OpenCV / FFmpeg / ccap

**Fit for VisionScope:** Rejected for the initial project phase; only reconsider if abstractions leak badly.

---

## Comparison summary

| Option | Cross-platform | Conan | Weight | Best when… | VisionScope recommendation |
|--------|----------------|-------|--------|------------|----------------------------|
| **SDL3 Camera** | Excellent | Yes (`sdl`) | Light–medium | Need frames + permissions, keep deps lean | Leading Proposed default |
| **OpenCV** | Excellent | Yes (`opencv`) | Heavy | Webcam reliability + future CV | Strong plan B |
| **FFmpeg** | Excellent | Yes (`ffmpeg`) | Medium–heavy | Capture + record/stream | Plan B/C for media features |
| **GStreamer** | Excellent | Yes (`gstreamer`) | Heavy | Pipelines / RTSP | Only if pipelines are core |
| **ccap** | Good (desktop) | Manual | Lightest dedicated | Minimal capture-only dep | Attractive if packaging OK |
| **libuvc** | Partial | Possible | Light | UVC USB only | Not sole backend |
| **DIY native** | Full (DIY) | N/A | Code-heavy | Absolute control | Too costly early |

## Consequences

### If we Accept SDL3 later

- Positive: lean Conan dependency; first-class permissions; simple frame loop for TUI adapters
- Negative: newer camera API; may need frame warm-up / format conversion helpers
- Follow-up: abstract behind `ICameraSource` so OpenCV/FFmpeg can be added without UI churn

### If we Accept OpenCV later

- Positive: faster path to capture reliability and optional CV features
- Negative: larger builds/binaries; more Conan option surface
- Follow-up: slim OpenCV Conan options to `videoio` (+ imaging essentials) where possible

### If we Accept FFmpeg later

- Positive: recording/encoding and advanced device strings become natural
- Negative: more complex lifecycle and license/build matrix
- Follow-up: clarify project licensing story for FFmpeg-enabled builds

### Regardless of backend

- Keep FTXUI and terminal graphics adapters independent of the capture library
- Document OS permission requirements (especially macOS camera entitlement / Info.plist usage when distributing .app bundles later)
- Prefer delivering frames in a stable internal pixel format (e.g. RGB24 or RGBA) at the adapter boundary

## Open questions

1. Is live **preview-only** the MVP, or is **recording** in scope for v1?
2. Do we need **network cameras / RTSP** early (pushes toward GStreamer/FFmpeg)?
3. How aggressive should we be about **binary size** vs. **device compatibility**?
4. Accept packaging a custom Conan recipe for **ccap**, or stay Conan-Center-only for v1?

## Next steps

1. Decide Accepted default backend (leaning SDL3 unless answers above change priorities).
2. Sketch `ICameraSource` + FTXUI shell wiring.
3. Spike device enumerate + one live frame path on macOS (maintainer platform), then Windows/Linux CI smoke later.
4. Supersede or update this ADR to **Accepted** with the chosen backend and rejected alternatives marked final.

## Related decisions

- Terminal image capability detection / graphics protocol selection — *not yet recorded* (future ADR).
- Conan + CMake project layout — *not yet recorded* (future ADR).
