# Changelog

## v2.3.1 — Print Config on Button 22

### Changed
- Moved the print-config trigger from **Button 3** (index 2) to **Button 22** (index 21) for the steering wheel layout.

### Fixed
- Hardware config now matches the steering wheel: `DIMENSION_6x4`, `ROTARY_ONLY_LAYERS` enabled, USB identity `0x255a:0xc613`.

## v2.3 — Rotary-Only Layer Mode

### Added
- `#define ROTARY_ONLY_LAYERS` compile-time toggle: when uncommented, buttons always output on Layer 1 regardless of the current layer. Rotary encoders still respond to layer changes normally.
- Long-press **Button 3** (index 2) for 10s to print full firmware configuration to Serial console (9600 baud). Output includes firmware version, dimension, button/rotary counts, controller ID, USB VID/PID/product name, layer selector assignments, and `ROTARY_ONLY_LAYERS` status.
- `BOX_VID`, `BOX_PID`, `BOX_PRODUCT` compile-time defines for USB identity (must match `boards.txt` values, see `DEVICES-MANAGEMENT.md`).
- `DIMENSION_STR` compile-time define for each matrix preset.
- `Serial.begin(9600)` in `setup()`.
- `printConfig()` helper function.

## v2.2 — Dynamic Layer Selector Assignment

### Added
- **Dynamic layer selector assignment** via long-press programming gestures:
  - `Button 1 + Button X` held 10s → assign Button X as Layer 2 selector
  - `Button 2 + Button Y` held 10s → assign Button Y as Layer 3 selector
  - `Button 1 + Button 2` held 10s → reset both selectors to unassigned (Layer 1 only)
- Selector indices persist in EEPROM (addresses 1 and 2) across power cycles
- Default: unassigned (255) — unprogrammed EEPROM (0xFF) naturally matches
- `PROG_HOLD_DURATION` constant (10000ms) for programming gesture hold time

### Changed
- `LAYER2_BUTTON_INDEX` and `LAYER3_BUTTON_INDEX` replaced with EEPROM-backed runtime variables `layer2Index` / `layer3Index`
- `CheckAllButtons` skips buttons matching runtime selector indices (suppressed during programming gestures)
- `UpdateLayer` uses runtime selector indices with early return when no selectors are assigned

### Removed
- Dead hold-to-press / momentary toggle feature (`buttonPressDuration`, `TOGGLE_BUTTON_DURATION_BTN`, `TOGGLE_HOLD_DURATION`, `EEPROM_BUTTON_DURATION_ADDR`)
- `blinkLED()` and `pinMode(LED_BUILTIN)` — LED is enclosed and invisible
- `buttonPressTimes[]` array — no longer needed
- Second pass (HOLD handler) in `CheckAllButtons` — all dead code

### Fixed
- Reset gesture (B1+B2) now works from WAIT states, not just PROG_IDLE
- Target button joystick output released when entering ARMED state (prevents stuck output)
- Blanket suppression of B1/B2 removed from `isProgrammingActive` — only target button is suppressed during ARMED states