# Architecture Decision Records

This directory tracks Architecture Decision Records (ADRs) for VisionScope.

## Format

Each ADR is a markdown file named `NNNN-short-title.md` and follows:

- **Status**: Proposed | Accepted | Deprecated | Superseded
- **Context**: Why the decision is needed
- **Decision**: What we chose (or are proposing)
- **Consequences**: Trade-offs and follow-ups
- **Alternatives considered**: Options evaluated and why they were not chosen (yet)

## Index

| ADR | Title | Status |
|-----|-------|--------|
| [0001](0001-cross-platform-camera-capture.md) | Cross-platform camera capture backend | Accepted (SDL3) |
| [0002](0002-terminal-graphics.md) | Terminal graphics protocol selection | Accepted |
| [0003](0003-conan-cmake-layout.md) | Conan 2 and CMake project layout | Accepted |
| [0004](0004-mvvm-structure.md) | MVVM application structure | Accepted |
| [0005](0005-tdd-solid-di.md) | TDD, SOLID, and reference-based DI | Accepted |
| [0006](0006-platform-acceleration.md) | Platform acceleration Strategy and folder layout | Accepted |

## Related docs (not ADRs)

| Doc | Purpose |
|-----|---------|
| [Upstreaming to Conan Center](../conan/upstreaming-to-conan-center.md) | How to contribute a third-party library recipe to Conan Center Index |
