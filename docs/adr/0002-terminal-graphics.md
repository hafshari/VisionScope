# ADR-0002: Terminal graphics protocol selection

- **Status:** Accepted
- **Date:** 2026-08-07
- **Deciders:** VisionScope maintainers
- **Tags:** tui, graphics, kitty, iterm2, sixel, wezterm, ftxui

## Context

VisionScope displays live camera frames in a terminal. Many modern emulators can render true pixel images via escape-sequence protocols; others only support Unicode/ANSI. FTXUI provides character-cell UI (menus, layout, input) but does **not** implement Kitty / iTerm2 / Sixel graphics ([FTXUI discussion #824](https://github.com/ArthurSonzogni/FTXUI/discussions/824)). VisionScope therefore needs its own capability detection and a thin graphics adapter beside FTXUI.

Target terminals of interest include iTerm2 and WezTerm (user-requested), plus Kitty/Ghostty and Sixel-capable hosts when available.

## Decision

**Accepted:** Detect terminal image capability at startup and select the best protocol from a fixed priority chain. Render frames through a `TerminalGraphicsAdapter` interface. Fall back to Unicode block / braille rendering when no bitmap protocol is available.

### Priority chain

1. **Kitty graphics protocol** — best quality (RGBA, animation-friendly); used by Kitty, Ghostty, and recent hosts that speak Kitty (including newer iTerm2 where supported)
2. **iTerm2 inline images** (`OSC 1337`) — good quality; iTerm2, WezTerm, VS Code terminal, mintty, and others
3. **Sixel** — wider legacy support; palette-based
4. **Unicode / block fallback** — FTXUI `Canvas` or half-block cells; always available

WezTerm typically supports iTerm2 (and often Kitty/Sixel depending on version); detection must prefer the highest protocol the host actually advertises, not a hard-coded TERM_PROGRAM → single protocol map.

### Detection strategy

Combine (in order of reliability):

1. **Environment heuristics for terminal identity** — read host-provided vars only (`TERM_PROGRAM`, `KITTY_WINDOW_ID`, `ITERM_SESSION_ID`, `WEZTERM_PANE`, `GHOSTTY_*`, etc.). These identify the emulator; they are not an app configuration API.
2. **Optional terminal queries** later (DA1 / XTGETTCAP) when heuristics are ambiguous
3. **CLI override** `--graphics=kitty|iterm2|sixel|unicode` sets the *initial* preferred protocol; omit or pass `auto` to keep detection. Detection still records every supported protocol for the runtime picker.

Do **not** use an app-specific env var (e.g. `VISIONSCOPE_GRAPHICS`) for protocol selection. Prefer auto-detect + CLI (same pattern as `timg -p` / `imgcat --protocol`) so behavior is explicit per invocation and not inherited from shell profiles. Runtime TUI selection covers interactive changes.

Never assume graphics support solely from `$TERM=xterm-256color`.

### Runtime selection

- Detection builds the **available** set: Unicode (always) plus each advertised bitmap protocol.
- CLI `--graphics=` picks the **startup** active protocol (and adds that protocol to available if heuristics missed it).
- The TUI exposes the available set (menu + `g` to cycle). ViewModel owns the mutable active protocol; the graphics adapter draws with that choice (falling back to Unicode half-blocks until Kitty/iTerm2/Sixel emitters exist).

### Integration with FTXUI

- FTXUI owns chrome: device list, protocol picker, status, key bindings.
- The graphics adapter writes protocol bytes on the main/UI thread after (or coordinated with) the FTXUI screen flush so image placement stays aligned with the preview region.
- ViewModels expose RGB/RGBA frames only; they never emit escape sequences ([ADR-0004](0004-mvvm-structure.md)).

## Consequences

- Positive: proper images in capable terminals (iTerm2, WezTerm, Kitty, …); graceful degradation elsewhere
- Negative: FTXUI + graphics need careful stdout coordination; protocol bugs are emulator-specific
- Follow-up: implement `detectTerminalCapabilities()` and stub adapters in the scaffold; flesh out Kitty/iTerm2 paths for the live-preview MVP

## Alternatives considered

| Option | Why not chosen as sole approach |
|--------|---------------------------------|
| FTXUI Canvas only | No true pixels; poor webcam preview quality |
| Switch UI stack to Notcurses | Strong graphics, but abandons chosen FTXUI stack |
| Always iTerm2 protocol | Misses Kitty-native hosts; fails on dumb terminals |
| Always assume no graphics | Ignores capable terminals the product targets |

## Related decisions

- [ADR-0001](0001-cross-platform-camera-capture.md) — Capture backend (SDL3)
- [ADR-0004](0004-mvvm-structure.md) — MVVM; graphics adapter lives in the View layer
