# ADR-0005: TDD, SOLID, and reference-based DI

- **Status:** Accepted
- **Date:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** tdd, solid, di, testing, architecture

## Context

VisionScope is growing past a scaffold: capture, graphics protocols, and FTXUI must stay swappable ([ADR-0001](0001-cross-platform-camera-capture.md), [ADR-0002](0002-terminal-graphics.md), [ADR-0004](0004-mvvm-structure.md)). Without tests and clear dependency rules, regressions and tight coupling will accumulate. The team also wants constructor injection aligned with **Boost.DI** conventions: prefer plain references (and values) over `unique_ptr` / `shared_ptr` in constructors.

## Decision

### Test-driven development (TDD)

1. For new behavior in **Model** and **ViewModel**, write a failing automated test first, then the minimal implementation, then refactor.
2. Prefer **unit tests** with fakes/stubs for `ICameraSource`, environment, and similar ports.
3. Keep FTXUI / SDL / protocol I/O behind seams so core logic is tested without a real terminal or camera when possible.
4. Test runner: **Google Test (gtest)** via Conan Center; CTest entry point `visionscope_tests`.

### SOLID (how we apply it)

| Principle | Application |
|-----------|-------------|
| **S** | One reason to change per type (`SdlCameraSource` ≠ `CameraViewModel` ≠ graphics adapter) |
| **O** | Extend via new `ICameraSource` / protocol adapters; avoid editing ViewModel for each backend |
| **L** | Fakes and SDL implementations honor `ICameraSource` contracts |
| **I** | Narrow ports (`ICameraSource`, `IEnvironment`) instead of kitchen-sink service bags |
| **D** | High-level modules depend on abstractions; composition root wires concretes |

### Dependency injection (Boost.DI-style)

- **Constructors take `T&` / `const T&` (or cheap values), not `std::unique_ptr<T>` / `std::shared_ptr<T>`.**
- **Ownership lives at the composition root** (`main` / app bootstrap): stack objects or a single owner scope outlives dependents.
- **Polymorphism via references to interfaces** (`ICameraSource&`), not smart-pointer members that imply ownership by the consumer.
- Smart pointers are allowed only where ownership transfer or shared lifetime is genuinely required (rare); not as the default DI vehicle.
- Optional later: Boost.DI (or similar) as the composition helper; the **reference-injection convention applies regardless** of whether a DI container is used.

Example (accepted):

```cpp
SdlCameraSource camera;                    // owned in main
CameraViewModel vm{camera, caps};          // borrows by reference
run_app(vm);
```

Example (rejected as default):

```cpp
CameraViewModel vm{std::make_unique<SdlCameraSource>(), caps};
```

### Concurrency and mutexes

Do **not** add locks “just in case.” Today `CameraViewModel::tick()` and `latest_frame()` both run on the FTXUI UI thread; the background timer only calls `PostEvent` ([ADR-0004](0004-mvvm-structure.md)).

If capture later runs on a worker thread and publishes frames to the UI:

1. Prefer [`safe/2.0.0`](https://conan.io/center/recipes/safe) (`safe::Safe<model::Frame>`) over a separate `std::mutex` + naked `Frame` — packs mutex and value so unlocked access is obvious (`unsafe()`) and hard to misuse.
2. Add it as a Conan dependency only when that cross-thread path exists (YAGNI until then).
3. Keep `std::atomic` (or similar) for simple flags such as “refresh thread should exit”; that is not a substitute for protecting `Frame` payloads.

## Consequences

- Positive: testable ViewModels; clear lifetimes; SOLID-friendly swaps of camera/env/graphics; no premature locking
- Negative: callers must ensure injected objects outlive the consumer (composition-root discipline)
- Follow-up: `visionscope_core` + gtest fakes (done); adopt `safe` when worker-thread capture lands

## Alternatives considered

| Option | Why not |
|--------|---------|
| `unique_ptr` everywhere for “clarity” | Fights Boost.DI style; muddies ownership vs use |
| `shared_ptr` for all services | Hides lifetime bugs; unnecessary sharing |
| UI-only / manual testing | Too slow and flaky for capture + protocol logic |
| Catch2 instead of gtest | Team standard is Google Test |
| Always depend on `safe` from day one | No shared mutable frame access yet; adds a dep for unused sync |

## Related decisions

- [ADR-0003](0003-conan-cmake-layout.md) — gtest as a Conan test dependency
- [ADR-0004](0004-mvvm-structure.md) — ViewModel depends on `ICameraSource&`; single-threaded frame path today
