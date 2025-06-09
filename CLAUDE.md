# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a DIY Sim Racing Button Box project that provides hardware templates, wiring diagrams, and component specifications for building a 32-function Arduino-based button box for sim racing games.

## Project Structure

- `buttonbox_layout.ai` - Vector template for drilling holes (real-size layout for 200x120mm case)
- `buttonbox_layout.png` - Visual preview of the layout template
- `buttonbox_wiring.drawio` - Wiring diagram source file (can be edited at diagrams.net)
- `buttonbox_wiring.jpeg` - Visual wiring diagram showing component connections
- `images/` - Product photos of all required components with specifications
- `README.md` - Complete build guide with component list, pricing, and Arduino setup instructions

## Hardware Architecture

The button box uses an Arduino Pro Micro ATmega32U4 as the main controller, connected to:
- 1x Engine start button (16mm momentary)
- 5x Toggle switches (MTS-123 on-off-on momentary)
- 2x Illuminated toggle switches (red and blue)
- 20x Push buttons (PBS-33B momentary, white and red)
- 5x Rotary encoders (EC11 with push button)

All components are housed in a 200x120x75mm waterproof case with carbon fiber vinyl finish.

## Software Dependencies

The project requires:
- Arduino IDE
- ArduinoJoystickLibrary (for HID joystick functionality)
- Keypad library by Mark Stanley and Alexander Brevig
- 32-FUNCTION-BUTTON-BOX sketch from AM-STUDIO

## Arduino Programming Setup

1. Board type: Arduino Leonardo (for Pro Micro compatibility)
2. Upload via USB cable connection
3. Verify sketch before uploading
4. Uses HID joystick protocol for game controller functionality

## File Modification Guidelines

- Layout template (`buttonbox_layout.ai`) should maintain real-world dimensions for drilling accuracy
- Wiring diagram should reflect actual pin connections to Arduino
- Component specifications in README must match actual hardware requirements
- Maintain drill bit size specifications (6, 7, 12, 14, 16 mm) in documentation