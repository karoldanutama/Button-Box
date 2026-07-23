# Changelog

## v2.5.1 — Restore Rotary Inputs in Rotary-Only Mode

### Fixed
- Restored Layer 2/3 rotary encoder inputs when `ROTARY_ONLY_LAYERS` is enabled. Encoder outputs now start immediately after the 24 base buttons (`24..47`) instead of being pushed into the high `72..95` range, which some games were not reading.

## v2.5 — Static Button Mapping

### Changed
- Button output mapping is now **static**: each physical button always outputs at its own index, regardless of which buttons are assigned as layer selectors. Previously, assigning a selector would compact the output slots, shifting all higher-indexed buttons down by one and requiring re-binding in games. Now, selector buttons simply produce no output — other buttons keep their original joystick button numbers.
- `NUM_ACTIVE_BUTTONS` removed; `ENCODER_BASE` now uses `NUMBUTTONS`

### Removed
- `outputSlotForKchar[]` array and its compaction loop in `setup()`

## v2.4 — Serial Feedback for Layer Programming

### Added
- Serial output on successful layer programming: "Layer 2 selector assigned to button X", "Layer 3 selector assigned to button Y", and "Layer selectors reset (unassigned)" messages printed to Serial console.

### Changed
- Programming gesture hold time reduced from 10s to 5s (`PROG_HOLD_DURATION` 10000 → 5000).

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
