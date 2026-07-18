# Devices Management

This file tracks every physical button box built from this repo (or its variants), each identified by a unique USB VID/PID/product name combo. Use this as the single source of truth when adding a new device, so no two boxes ever collide on the same PC.

**Table of Contents**

1. [How to Change build.vid, build.pid, and Device Name](#how-to-change-buildvid-buildpid-and-device-name)
2. [Device Mapping Table](#device-mapping-table)

## How to Change build.vid, build.pid, and Device Name

By default, uploading via `Tools > Board > Arduino Leonardo` makes Windows/the sim see the device as a generic "Arduino Leonardo." To make it identify with a custom name (and optionally a custom VID/PID) when plugged in, edit the board's USB descriptor **before compiling** — this is not something you change in the `.ino` sketch itself.

**File to edit:**

```
C:\Users\<you>\AppData\Local\Arduino15\packages\arduino\hardware\avr\<version>\boards.txt
```

(Path varies slightly by Arduino IDE version — if it's not there, start from `AppData` and look for `Arduino15\packages\arduino\hardware\avr\`.)

**Lines to change**, under the `leonardo.*` section (or `micro.*` if you're compiling as "Arduino Micro" instead — check which one you actually select in `Tools > Board`):

```
leonardo.build.vid=0x2341
leonardo.build.pid=0x8036
leonardo.build.usb_product="Your Device Name Here"
```

- `vid`/`pid` are the USB Vendor/Product ID reported to Windows. These don't need to be officially registered for a personal project — just pick something unique so Windows and the sim can tell multiple boxes apart in Device Manager and the controller list.
- `usb_product` is the actual string Windows and games display as the device name. This is what you'll see in the joystick binding list.

**Steps:**

1. Close the Arduino IDE if it's open.
2. Back up `boards.txt` before editing, in case an IDE update or reinstall overwrites it later.
3. Edit the three lines above under the correct board section.
4. Reopen Arduino IDE, re-select `Tools > Board > Arduino Leonardo` (this reloads the edited file).
5. Re-upload the sketch for that device.
6. Unplug/replug the USB cable — Windows should now list the device under the new name in Game Controllers and in the sim's control binding screen.
7. **Add an entry to the [Device Mapping Table](#device-mapping-table) below** so the VID/PID/name is recorded and won't accidentally get reused.

**Note:** This edit lives in your local Arduino installation, not in this repo — reinstalling or updating the Arduino IDE will likely reset `boards.txt` to default, so refer back to this table to reapply all your devices' identities if that happens.

## Device Mapping Table

| VID | PID | Device Name | Layout | Buttons | Shift Button Index | Description |
|-----|-----|-------------|--------|---------|---------------------|--------------|
| `0x256b` | `0xc618` | Akamai Box 5x5 - 64 Buttons | 5x5 matrix | 64 | 8 | Simple button box with a 5x5 pin layout, firmware-expanded to 64 joystick buttons. |
| `0x256a` | `0xc617` | Akamai Box v2 5x5 - 64 Buttons | 5x5 matrix | 64 | 22 | DIY button box with 5 three-way toggle switches, 8 push buttons, 1 self-lock start button, 1 aviation toggle switch with safety cover (ignition), and 4 rotary encoders with pushable button. Physical inputs are firmware-expanded to 64 joystick buttons. |
