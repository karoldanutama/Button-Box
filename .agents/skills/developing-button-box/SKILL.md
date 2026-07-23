---
name: developing-button-box
description: Instructions for developing the Button Box firmware. Use when making code changes, creating PRs, bumping versions, or preparing firmware for deployment to specific hardware.
---

# Developing the Button Box

## Overview

This is a DIY Sim Racing Button Box project — an Arduino Pro Micro / ATmega32U4 HID joystick firmware with a layered input model, rotary encoders, dynamic selector assignment, and per-device USB identity management.

The firmware lives in `button_box/button_box.ino`. All development follows the workflow below.

---

## 1. Conventional Commits

Every PR must follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) format:

```
<type>: <description>
```

**Types:**

| Type | Usage | Version bump |
|------|-------|-------------|
| `feat:` | New feature | MINOR |
| `fix:` | Bug fix | PATCH |
| `docs:` | Documentation only | none |
| `chore:` | Maintenance, tooling | none |
| `refactor:` | Code change without feature/fix | none |

**Breaking changes** use `!` after the type or a `BREAKING CHANGE:` footer — triggers a MAJOR bump.

**Examples:**
```
feat: add ROTARY_ONLY_LAYERS compile-time toggle
fix: reset gesture now works from WAIT states
docs: update README with config output section
```

---

## 2. CHANGELOG.md

Every PR must update `CHANGELOG.md` with an entry under the appropriate version heading.

**Format:**
```markdown
## vX.Y.Z — Title

### Added
- ...

### Changed
- ...

### Removed
- ...

### Fixed
- ...
```

If the version section doesn't exist yet, create it at the top of the file.

---

## 3. Version Bumping

Based on the conventional commit type, bump `#define FIRMWARE_VERSION` in `button_box/button_box.ino`:

| Commit type | Example | Version change |
|-------------|---------|---------------|
| `feat:` | `feat: add toggle` | 2.3 → **2.4** (MINOR) |
| `fix:` | `fix: gesture bug` | 2.3 → **2.3.1** (PATCH) |
| `BREAKING CHANGE:` or `!` | `feat!: drop support` | 2.3 → **3.0** (MAJOR) |
| `docs:`, `chore:`, `refactor:` | `docs: update` | no bump |

The constant is near the top of the sketch:

```cpp
#define FIRMWARE_VERSION "2.3"
```

Update it to the new version, e.g.:

```cpp
#define FIRMWARE_VERSION "2.4"       // MINOR bump
#define FIRMWARE_VERSION "2.3.1"     // PATCH bump
#define FIRMWARE_VERSION "3.0"       // MAJOR bump
```

---

## 4. Hardware Deployment Configuration

The firmware is deployed to various types of hardware. Before deploying, configure these `#define` values near the top of `button_box/button_box.ino`:

| Define | Purpose | Example |
|--------|---------|---------|
| `DIMENSION_*` | Matrix preset | `#define DIMENSION_5x5` |
| `ROTARY_ONLY_LAYERS` | Uncomment for rotary-only mode | `#define ROTARY_ONLY_LAYERS` |
| `BOX_VID` | USB Vendor ID (must match `boards.txt`) | `"0x256a"` |
| `BOX_PID` | USB Product ID (must match `boards.txt`) | `"0xc615"` |
| `BOX_PRODUCT` | USB device name (must match `boards.txt`) | `"Akamai Box 5x5"` |
| `CONTROLLER_ID` | Internal HID report ID | `2` |

**Device registration:** After configuring a device, add an entry to the table in `DEVICES-MANAGEMENT.md` with all columns filled (`VID`, `PID`, `Device Name`, `Layout`, `Buttons`, `Shift Button Index`, `Firmware Version`, `Rotary Only Layers`, `Description`).

---

## 5. Key Files

| File | Purpose |
|------|---------|
| `button_box/button_box.ino` | Firmware source. Constants at the top, version at `FIRMWARE_VERSION`. |
| `CHANGELOG.md` | Version history. Updated with every PR. |
| `DEVICES-MANAGEMENT.md` | Per-device registry with USB identity, layout, and firmware version. |
| `README.md` | General documentation and architecture overview. |