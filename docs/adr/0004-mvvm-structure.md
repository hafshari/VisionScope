# ADR-0004: MVVM application structure

- **Status:** Accepted
- **Date:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** architecture, mvvm, ftxui, tui

## Context

VisionScope combines three concerns that change at different rates:

1. Camera capture (SDL3 today; swappable per [ADR-0001](0001-cross-platform-camera-capture.md))
2. Terminal bitmap protocols ([ADR-0002](0002-terminal-graphics.md))
3. Interactive TUI chrome (FTXUI)

Mixing capture, escape sequences, and FTXUI widgets in one translation unit makes backends hard to swap and graphics hard to test. Classic MVVM (Model–View–ViewModel) fits a desktop TUI: the ViewModel holds presentation state and commands; the View binds to it; the Model owns I/O.

## Decision

**Accepted:** Structure the application as MVVM with the following boundaries.

```text
Model                    ViewModel                 View
─────                    ─────────                 ────
ICameraSource            CameraViewModel           FTXUI components
Frame (RGB/RGBA)         selected device           TerminalGraphicsAdapter
TerminalCapabilities     running / error / frame   (Kitty / iTerm2 / …)
FrameBuffer              commands (start/stop/…)
```

### Responsibilities

| Layer | Owns | Must not own |
|-------|------|--------------|
| **Model** | Device enumerate/open/grab/close; frame buffer; capability detection result | FTXUI types; escape sequences; user-facing strings beyond raw errors |
| **ViewModel** | Selected device index/name; run state; latest frame for display; commands; status text | SDL/FTXUI includes; protocol bytes |
| **View** | FTXUI layout/events; calling graphics adapter with ViewModel frames | Capture backend details |

### Key types (scaffold)

- `visionscope::model::ICameraSource` — abstract capture
- `visionscope::model::SdlCameraSource` — SDL3 implementation (may be stubbed until live MVP)
- `visionscope::model::Frame` — width, height, RGB24/RGBA bytes
- `visionscope::model::TerminalCapabilities` — detected protocol flags
- `visionscope::viewmodel::CameraViewModel` — state + commands; constructed with `ICameraSource&` + `const TerminalCapabilities&` (no smart-pointer DI; see [ADR-0005](0005-tdd-solid-di.md))
- `visionscope::view::App` — FTXUI screen loop
- `visionscope::view::TerminalGraphicsAdapter` — protocol dispatch / Unicode fallback

### Data flow

```text
User input (FTXUI)
      │
      ▼
CameraViewModel.commands
      │
      ▼
ICameraSource ──► Frame ──► CameraViewModel.latestFrame
                              │
              ┌───────────────┴───────────────┐
              ▼                               ▼
     FTXUI chrome/status          TerminalGraphicsAdapter
```

Capture acquisition runs on the **UI thread** today (`tick()` from the FTXUI renderer). The refresh helper thread only posts `Event::Custom`; it does not touch ViewModel state. Therefore **no frame mutex** is required for the scaffold.

When (and only when) grab moves to a worker thread, protect `latest_frame_` with [`safe/2.0.0`](https://conan.io/center/recipes/safe) (`safe::Safe<model::Frame>`), not a bare `std::mutex` beside a naked `Frame` — see [ADR-0005](0005-tdd-solid-di.md) concurrency note.

## Consequences

- Positive: SDL3 ↔ OpenCV/ccap swap without UI rewrite; graphics protocols testable without a camera; clear package layout under `src/model|viewmodel|view`
- Negative: Slightly more boilerplate than a single-file demo
- Follow-up: live graphics protocols; optional worker-thread capture + `safe` if UI blocking becomes an issue

## Alternatives considered

| Option | Why not |
|--------|---------|
| MVC with fat controller | Controllers tend to absorb FTXUI + capture; weaker for reactive frame updates |
| No layering (spike forever) | Fine for a 100-line demo; fails once protocols + multi-camera land |
| Full reactive framework | Overkill for a focused CLI tool |
| Always mutex / always `safe` around frames | Premature; current access is single-threaded on the UI path |

## Related decisions

- [ADR-0001](0001-cross-platform-camera-capture.md) — `ICameraSource` + SDL3
- [ADR-0002](0002-terminal-graphics.md) — Graphics adapter in View
- [ADR-0003](0003-conan-cmake-layout.md) — Directory layout
- [ADR-0005](0005-tdd-solid-di.md) — TDD / SOLID / DI; concurrency + `safe`