# ADR-0003: Conan 2 and CMake project layout

- **Status:** Accepted
- **Date:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** conan, cmake, build, packaging, ftxui, sdl

## Context

VisionScope is a C++ desktop TUI. Dependencies must be reproducible across macOS, Linux, and Windows. Tooling versions are already pinned in [`requirements.txt`](../../requirements.txt) (`cmake`, `conan==2.30.0`, `ninja`). ADR-0001 accepts SDL3 from Conan Center; FTXUI is also on Conan Center. Plan-B libraries such as **ccap** are not on Conan Center and would need a local recipe.

## Decision

**Accepted layout:**

```text
VisionScope/
├── CMakeLists.txt              # root project + targets
├── conanfile.py                # requires + generators (Conan 2)
├── requirements.txt            # pip: cmake, conan, ninja
├── cmake/                      # helpers if needed
├── src/
│   ├── main.cpp                # composition root
│   ├── model/
│   ├── viewmodel/
│   └── view/
│       ├── accel/              # portable Strategy defaults + factory
│       └── platform/           # OS packs; each has CMakeLists.txt
│           └── apple/          # add_subdirectory only on APPLE
├── include/visionscope/        # public/internal headers (as needed)
├── recipes/                    # local Conan recipes (e.g. ccap) when required
└── docs/
```

### Dependency management

| Dependency | Source | Notes |
|------------|--------|--------|
| **ftxui** | Conan Center | TUI |
| **sdl** | Conan Center | Camera API; set `camera=True` (and disable unused SDL subsystems where practical) |
| **gtest** | Conan Center (`test_requires`) | Unit tests ([ADR-0005](0005-tdd-solid-di.md)); toggle via `with_tests` |
| **ccap** (plan B only) | Local `recipes/ccap/` then optional CCI PR | Not required for default SDL3 stack |

### Conan 2 usage

- Use a `conanfile.py` (or `conanfile.txt`) with `CMakeDeps` + `CMakeToolchain` generators and `cmake_layout`.
- Typical flow:

```bash
python -m pip install -r requirements.txt
conan install . --output-folder=build --build=missing -s build_type=Release
cmake --preset conan-release   # or -DCMAKE_TOOLCHAIN_FILE=...
cmake --build --preset conan-release
```

Exact preset names may follow Conan’s generated presets; document the canonical commands in the README.

### Local recipes (ccap path)

If/when ADR-0001’s plan B is activated:

1. Add `recipes/ccap/` in CCI-compatible layout (`config.yml`, `all/conanfile.py`, `conandata.yml`, `test_package/`).
2. Create/export the package locally (`conan create recipes/ccap/all …`) or add a path remote.
3. Optionally upstream to [conan-center-index](https://github.com/conan-io/conan-center-index) per [Upstreaming to Conan Center](../conan/upstreaming-to-conan-center.md).

Until then, **do not** vendor ccap; stay Conan-Center-only for the default stack.

### CMake targets

- One executable target: `visionscope`
- Link privately: `ftxui::screen`, `ftxui::dom`, `ftxui::component`, and SDL3 camera-capable targets (`SDL3::SDL3` or recipe-provided names)
- C++ standard: **C++20** (or C++17 minimum if a dep forces it; prefer 20 for the app)

## Consequences

- Positive: reproducible deps; matches Conan-first decision in ADR-0001; clear place for a future ccap recipe
- Negative: Contributors need Conan 2 + a profile; first builds may compile SDL from source if no binary matches
- Follow-up: Scaffold `conanfile.py` + root `CMakeLists.txt` + README build section

## Alternatives considered

| Option | Why not |
|--------|---------|
| FetchContent-only | Weaker versioning/cache story across platforms |
| vcpkg as primary | Team already chose Conan + pinned tooling |
| System packages only | Inconsistent across macOS/Windows/Linux |
| Vendor SDL/FTXUI | Unnecessary given Conan Center coverage |

## Related decisions

- [ADR-0001](0001-cross-platform-camera-capture.md) — SDL3 default; ccap packaging plan B
- [ADR-0004](0004-mvvm-structure.md) — Source tree mirrors Model / ViewModel / View
