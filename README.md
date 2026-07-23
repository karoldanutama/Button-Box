# DIY Sim Racing Button Box

Arduino Pro Micro button box firmware and build assets for a custom sim racing control box.

This repository started as a fork of AMSTUDIO's 32-function button box project, but the firmware in this repo is now its own implementation track. The current sketch is a layered HID joystick firmware built around a matrix-scanned button panel, rotary encoders, and per-device USB identity management.

## Features

- Arduino Pro Micro / ATmega32U4 HID joystick firmware
- Default `5x5` matrix configuration with multiple matrix presets in the codebase
- `3-layer` input model with **dynamic layer selector assignment** via long-press programming gestures
- Silent layer-selector inputs that do not consume joystick button outputs
- `4` rotary encoders with per-layer clockwise and counter-clockwise actions
- `#define ROTARY_ONLY_LAYERS` compile-time toggle to restrict layering to encoders only
- Long-press **Button 3** for 10s to print full firmware configuration to Serial console
- Up to `96` declared joystick buttons, with `93` actively used in the default build
- Compact logical-to-joystick button remapping that skips reserved selector positions
- Safe button release handling across mid-press layer changes
- Custom controller report IDs for running multiple boxes on one system
- Documented USB VID/PID/product-name management workflow via `DEVICES-MANAGEMENT.md`

Credits to the original projects this build was based on:

- [AM-STUDIO / 32-FUNCTION-BUTTON-BOX](https://github.com/AM-STUDIO/32-FUNCTION-BUTTON-BOX)
- [OpenSimHardware DIY Arduino Button Box](https://opensimhardware.wordpress.com/diy-arduino-buttonbox-version-1-eng/)

## What This Repo Is

This repo contains three things:

- firmware in `button_box/button_box.ino`
- physical build reference assets such as the drill template and wiring diagram
- device identity documentation in `DEVICES-MANAGEMENT.md`

The important distinction is that the firmware is now the primary source of truth. The original fork description focused on a simpler upstream button box. That is no longer accurate for this codebase.

## Current Firmware At A Glance

The default configuration in `button_box/button_box.ino` is:

- MCU: Arduino Pro Micro / ATmega32U4
- USB mode: HID joystick via `ArduinoJoystickLibrary`
- Matrix preset: `DIMENSION_5x5`
- Physical matrix positions: `25`
- Reserved matrix positions: `2` for layer selection (dynamically assignable)
- Active matrix inputs: `23`
- Rotary encoders: `4`
- Layers: `3`
- Declared joystick buttons: `96`
- Actively used joystick buttons: `93`
- Firmware version: queryable via Button 3 long-press (Serial, 9600 baud)

## Source Of Truth

Use the repo in this order:

1. `button_box/button_box.ino` for actual runtime behavior
2. `DEVICES-MANAGEMENT.md` for USB identity allocation and per-device tracking
3. `buttonbox_layout.ai` and `buttonbox_wiring.drawio` for physical reference assets

## Firmware Architecture

The sketch is organized around four main responsibilities:

1. Compile-time layout selection
2. Matrix scanning and button state handling
3. Layer-aware output mapping
4. Rotary encoder decoding and pulse generation

At runtime, the main loop is intentionally simple:

```cpp
void loop() {
  buttbx.getKeys();
  UpdateLayer();
  CheckAllEncoders();
  CheckProgrammingGestures();
  CheckAllButtons();
}
```

That sequence matters:

- `getKeys()` refreshes the matrix scan once per loop
- `UpdateLayer()` derives the current layer from the live selector button states
- `CheckAllEncoders()` emits encoder pulses using the current layer
- `CheckProgrammingGestures()` runs the long-press state machine for layer selector assignment, reset, and version output
- `CheckAllButtons()` translates matrix events into joystick button state changes

## Compile-Time Configuration Strategy

Most behavior is selected by preprocessor defines near the top of the sketch.

### Controller Identity In The HID Report Layer

Each flashed box should get its own `CONTROLLER_ID`:

```cpp
#define CONTROLLER_ID 2
```

This does not change the USB device name by itself. It changes the joystick report ID used by the `Joystick` library so multiple controllers do not collide internally.

Current mapping in the sketch:

- `1` -> report ID `500`
- `2` -> report ID `510`
- `3` -> report ID `520`
- `4` -> report ID `530`
- fallback -> report ID `540`

USB VID/PID/product-name management is handled separately in `DEVICES-MANAGEMENT.md`.

### Matrix Presets

The sketch still carries multiple matrix presets:

- `DIMENSION_3x11`
- `DIMENSION_6x5`
- `DIMENSION_6x4`
- `DIMENSION_7x3`
- `DIMENSION_5x5`

Each preset determines:

- `NUMROWS`
- `NUMCOLS`
- `NUMROTARIES`
- `NUMBUTTONS`
- `rowPins[]`
- `colPins[]`
- the `buttons[][]` logical key map

The current default is:

```cpp
#define DIMENSION_5x5
```

For the 5x5 preset, the row/column wiring in the sketch is:

```cpp
byte rowPins[NUMROWS] = {21,20,19,18,15};
byte colPins[NUMCOLS] = {14,16,10,9,8};
```

## Matrix Mapping Strategy

The Keypad library works with logical key codes stored in the `buttons[][]` array. In the default 5x5 configuration, the matrix is mapped to logical button indices `0..24`.

```cpp
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23,24},
};
```

This repo also fixes a bug present in an earlier 5x5 mapping where the last row repeated `20`, which made logical button `24` unreachable.

## Layering Strategy

The firmware supports a 3-layer system with **dynamic layer selector assignment**.

### Dynamic Selector Assignment

Layer selectors are no longer hardcoded to specific matrix positions. Users assign which physical buttons act as layer toggles via long-press programming gestures:

| Gesture | Duration | Result |
|---------|----------|--------|
| Button 1 + Button X | 10s | Assign Button X as Layer 2 selector |
| Button 2 + Button Y | 10s | Assign Button Y as Layer 3 selector |
| Button 1 + Button 2 | 10s | Reset both selectors to unassigned |

Selector assignments persist in EEPROM (addresses 1 and 2) across power cycles. Default is unassigned (255) — all 25 buttons are active, Layer 1 only.

### Rotary-Only Layer Mode

When `#define ROTARY_ONLY_LAYERS` is uncommented, buttons always output on Layer 1 regardless of the current layer. Only rotary encoders respond to layer changes. This is useful for devices where physical buttons should remain consistent across modes (e.g., a steering wheel where buttons are fixed but encoders are mode-dependent).

### Why Selectors Are Silent

Selector buttons never fire their own joystick output. They are silent modifiers: activating a layer via a held button shouldn't also hold a joystick button down the entire time.

### Runtime Layer Resolution

`UpdateLayer()` walks the Keypad state list and checks whether either assigned selector input is currently `PRESSED` or `HOLD`. If no selectors are assigned (both 255), the firmware stays on Layer 1. If both selectors are somehow active, Layer 3 wins (defensive priority chain).

## Output Numbering Strategy

The firmware separates physical inputs from joystick output numbers.

### Step 1: Exclude Layer Selectors From Normal Button Output

In the default 5x5 setup there are `25` logical matrix positions, but two are reserved for the layer switch.

```cpp
#define NUM_ACTIVE_BUTTONS (NUMBUTTONS - 2)
```

So only `23` matrix positions generate normal joystick outputs.

### Step 2: Compact The Remaining Physical Buttons

During `setup()`, the sketch builds `outputSlotForKchar[]` from the EEPROM-loaded selector indices.

This converts each physical/logical matrix key code into a compact output slot from `0..22`, skipping the two selector indices (whatever they are assigned to).

That means joystick numbering does not have holes just because two physical positions are dedicated to layer selection.

### Step 3: Expand Per Layer

When a button is pressed, its joystick output is computed as:

```cpp
outputButton = slot + (currentLayer - 1) * NUM_ACTIVE_BUTTONS;
```

In the default build:

- Layer 1 matrix outputs: `0..22`
- Layer 2 matrix outputs: `23..45`
- Layer 3 matrix outputs: `46..68`

### Step 4: Reserve A Separate Encoder Range

Encoders are allocated after all matrix-derived outputs:

```cpp
#define ENCODER_BASE (NUM_ACTIVE_BUTTONS * 3)
```

For the default build that means:

- matrix outputs occupy `0..68`
- encoder outputs occupy `69..92`

`TOTAL_JOYSTICK_BUTTONS` is declared as `96`, leaving a small amount of headroom above the `93` actively used outputs.

## Why Releases Stay Correct Across Layer Changes

One subtle problem with layered input systems is this:

- press a button on Layer 1
- change to Layer 2 while still holding it
- release the physical button

If the firmware recomputed the output number on release, it might accidentally release the Layer 2 output instead of the Layer 1 output that was actually pressed.

This sketch avoids that bug by storing the real joystick output index per physical key in `activeOutputButton[]`.

On press:

- the current layer is sampled
- the final joystick output index is computed
- that output index is stored in `activeOutputButton[kchar]`
- the joystick button is set to `1`

On release or idle transition:

- the sketch clears the exact stored output index
- it does not recompute from the current layer

This is one of the more important implementation details in the fork because it keeps layered behavior stable even when the operator changes modes mid-press.

## Button Event Handling Strategy

`CheckAllButtons()` handles matrix-derived button outputs in a single pass.

The pass looks at `stateChanged` entries from the Keypad list.

Behavior:

- selector keys are skipped completely
- buttons involved in an active programming gesture are suppressed
- on `PRESSED`, the sketch asserts the joystick output for the current layer (or Layer 1 if `ROTARY_ONLY_LAYERS` is enabled)
- on `RELEASED` or `IDLE`, the sketch releases the previously stored output if one is active

This is effectively standard hold-to-press behavior.

## Rotary Encoder Strategy

Rotary encoders are handled separately from the matrix.

### Pin Allocation

The default build declares `4` encoders:

```cpp
#define MAX_ROTARIES 4
```

The current encoder pin pairs are:

- encoder 1: pins `0`, `1`
- encoder 2: pins `2`, `3`
- encoder 3: pins `4`, `5`
- encoder 4: pins `6`, `7`

### Per-Layer Output Allocation

Each encoder has six assigned output numbers:

- CCW on Layer 1
- CW on Layer 1
- CCW on Layer 2
- CW on Layer 2
- CCW on Layer 3
- CW on Layer 3

That is why `4` encoders consume `24` joystick outputs total.

### Decoder Implementation

The sketch uses a quadrature state table `ttable` and a small per-encoder state byte. This is a standard finite-state approach for rejecting invalid transitions and determining whether a full step was clockwise or counter-clockwise.

At runtime:

1. `rotary_process()` reads the two encoder pins
2. it advances the state machine using `ttable`
3. it returns `DIR_CCW`, `DIR_CW`, or no completed step
4. `CheckAllEncoders()` selects the output pair for the current layer
5. the chosen joystick button is pulsed with a `50 ms` press

That last point is important: encoders are not held. They emit short button pulses so games see each detent as a discrete action.

## Input Electrical Behavior

The sketch enables pull-ups:

```cpp
#define ENABLE_PULLUPS
```

Encoder inputs are initialized as `INPUT`, then driven high to enable the MCU's internal pull-up resistors.

The matrix scanning itself is handled by the Keypad library using the configured row/column pins.

## Device Identity Management

There are two different identity layers to keep in mind.

### 1. Internal Report Identity

This is controlled by `CONTROLLER_ID` -> `JOYSTICK_REPORT_ID` in the sketch.

### 2. USB Identity Seen By Windows And Games

This is controlled outside the sketch by editing the Arduino core's `boards.txt`, as documented in `DEVICES-MANAGEMENT.md`.

That file tracks:

- VID
- PID
- device name
- layout
- button count
- shift/layer mapping
- device description

If you run multiple button boxes on one PC, this file is the operational registry that prevents collisions and confusion in Device Manager and in-game controller lists.

## Build Assets And Their Limits

The repo still includes the original physical build assets.

### Layout Template

[![Layout Template](/buttonbox_layout.png)](/buttonbox_layout.ai)
[Download vector template](/buttonbox_layout.ai)

This is a real-size drilling template for a `200 x 120 mm` case.

The original documented drill sizes are:

- `6 mm`
- `7 mm`
- `12 mm`
- `14 mm`
- `16 mm`

### Wiring Diagram

[![Wiring Diagram](/buttonbox_wiring.jpeg)](/buttonbox_wiring.drawio)
[Download drawio diagram](/buttonbox_wiring.drawio)

You can edit the source in [diagrams.net](https://app.diagrams.net).

These files remain useful, but they should be treated as physical reference material, not as a full specification of the current firmware behavior.

## Viewing Current Configuration

Hold **Button 3** (matrix index 2) for 10 seconds to print the full firmware configuration to the Serial console at 9600 baud:

```
--- Button Box Config ---
Firmware: v2.3
Dimension: 5x5
Buttons: 25
Rotaries: 4
Controller ID: 2
Joystick report ID: 510
Layer 2 selector: unassigned
Layer 3 selector: unassigned
Rotary-only layers: false
--------------------------
```

To view this output:

1. Connect the Pro Micro to your PC via USB
2. In Arduino IDE, select **Tools → Serial Monitor** (or `Ctrl+Shift+M`)
3. Set the baud rate to **9600** in the bottom-right dropdown
4. Hold **Button 3** for 10 seconds

This is useful for verifying which firmware version and configuration is flashed on a device without opening the enclosure. The version string is defined by `FIRMWARE_VERSION` near the top of the sketch.

## Arduino Setup

### Required Libraries

Install these before compiling:

- [ArduinoJoystickLibrary](https://github.com/MHeironimus/ArduinoJoystickLibrary)
- `Keypad` by Mark Stanley and Alexander Brevig

### Flashing Steps

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the Joystick library.
3. Install the `Keypad` library from the Arduino Library Manager.
4. Open `button_box/button_box.ino` in Arduino IDE.
5. Set `CONTROLLER_ID` for the device you are flashing.
6. Confirm the selected matrix preset and any custom pin/layout edits.
7. Connect the Pro Micro over USB.
8. In Arduino IDE, select `Tools > Board > Arduino Leonardo`.
9. Select the correct port under `Tools > Port`.
10. Verify the sketch.
11. Upload the sketch.

If you also need custom USB naming, apply the `boards.txt` changes from `DEVICES-MANAGEMENT.md` before compiling.

## Supported And Tracked Device Variants

`DEVICES-MANAGEMENT.md` currently tracks these example device identities:

- `Akamai Box 5x5 - 64 Buttons` (v2.1)
- `Akamai Box 5x5 - 96 Buttons` (v2.1)
- `Akamai Box 5x5 v2.1 - 64 Buttons` (v2.1)
- `Akamai Steering Wheel 6x4 v2.3 - 96 Buttons` (v2.3, rotary-only layers)

Those entries document deployed variants. They are not all direct descriptions of the exact default sketch in this repo at this moment.

## Compatibility

The sketch comments mention testing with:

- Windows 10
- Assetto Corsa

Since the board presents itself as a standard HID joystick, the same strategy should work with other sims and games that support generic game controllers.

## Practical Guidance For Further Changes

If you extend this firmware, the safest order is:

1. decide the physical matrix layout and pin map
2. update the matrix preset and `buttons[][]` mapping
3. reserve any silent modifier positions before assigning joystick outputs
4. verify layer math and encoder output ranges do not overlap
5. assign a unique `CONTROLLER_ID`
6. register a unique VID/PID/device name in `DEVICES-MANAGEMENT.md`

The layered output model is compact and flexible, but most regressions in this kind of firmware come from mismatched assumptions between physical matrix positions, reserved modifier keys, and joystick output numbering. The current implementation is structured specifically to keep those concerns separated.
