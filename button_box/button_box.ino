//BUTTON BOX
//USE w ProMicro
//Tested in WIN10 + Assetto Corsa
//AMSTUDIO
//20.8.17
//
// --- MODIFIED: Dynamic layer selector assignment ---
// Layer selectors are no longer hardcoded. Users assign which physical
// buttons act as layer toggles via long-press programming gestures:
//   B1 + BX held 10s  -> BX becomes Layer 2 selector
//   B2 + BY held 10s  -> BY becomes Layer 3 selector
//   B1 + B2 held 10s  -> reset both selectors (no layers, 25 active buttons)
// Selector assignments persist in EEPROM across power cycles.
// Default: unassigned (255) -- all 25 buttons are active, Layer 1 only.
//
// Selector buttons never fire their own joystick output. They are silent
// modifiers: activating a layer via a held button shouldn't also hold a
// joystick button down the entire time.
//
// Fixed a pre-existing bug in the 5x5 button map: the last row repeated
// "20" instead of using "24", meaning physical button 24 never worked.

#include <Keypad.h>
#include <Joystick.h>
#include <EEPROM.h>

// Controller Configuration -- Change this before deploy
#define CONTROLLER_ID 2 // Change this to 1 for the second controller, 2 for third, etc.
#define DIMENSION_5x5

// Use completely different report IDs to avoid conflicts
#if CONTROLLER_ID == 1
  #define JOYSTICK_REPORT_ID 500
#elif CONTROLLER_ID == 2
  #define JOYSTICK_REPORT_ID 510
#elif CONTROLLER_ID == 3
  #define JOYSTICK_REPORT_ID 520
#elif CONTROLLER_ID == 4
  #define JOYSTICK_REPORT_ID 530
#else
  #define JOYSTICK_REPORT_ID 540
#endif

#define ENABLE_PULLUPS
#define MAX_ROTARIES 4 // Max number of rotaries

#if defined(DIMENSION_3x11)
  #define DIMENSION_STR "3x11"
  #define NUMROWS 3
  #define NUMCOLS 11
  #define NUMROTARIES 0
  #define NUMBUTTONS 32
#elif defined(DIMENSION_6x5)
  #define DIMENSION_STR "6x5"
  #define NUMROWS 6
  #define NUMCOLS 5
  #define NUMROTARIES 3
  #define NUMBUTTONS 26
#elif defined(DIMENSION_6x4)
  #define DIMENSION_STR "6x4"
  #define NUMROWS 6
  #define NUMCOLS 4
  #define NUMROTARIES 4
  #define NUMBUTTONS 24
#elif defined(DIMENSION_7x3)
  #define DIMENSION_STR "7x3"
  #define NUMROWS 7
  #define NUMCOLS 3
  #define NUMROTARIES 4
  #define NUMBUTTONS 21
#elif defined(DIMENSION_5x5)
  #define DIMENSION_STR "5x5"
  #define NUMROWS 5
  #define NUMCOLS 5
  #define NUMROTARIES 4
  #define NUMBUTTONS 25
#else
  #define DIMENSION_STR "5x5"
  #define NUMROWS 5
  #define NUMCOLS 5
  #define NUMROTARIES 4
  #define NUMBUTTONS 25
#endif

// --- LAYER CONFIG ---
// Layer selectors are runtime-configurable via long-press programming
// gestures (see CheckProgrammingGestures). Indices are persisted in EEPROM.
// Default: 255 (unassigned) -- all 25 buttons are active, Layer 1 only.
//
// EEPROM addresses for persistent selector configuration.
#define EEPROM_LAYER2_ADDR 1
#define EEPROM_LAYER3_ADDR 2
#define PROG_HOLD_DURATION 10000 // Hold duration in ms for programming gestures
#define FIRMWARE_VERSION "2.3"
#define VERSION_BUTTON 2 // Button 3 (index 2) held 10s prints version to Serial

// When defined, only rotary encoders respond to layer changes.
// Buttons always output on Layer 1 regardless of the current layer.
// Uncomment this line to enable rotary-only layer mode.
// #define ROTARY_ONLY_LAYERS

// Number of active matrix buttons per layer (always 2 less than total,
// since 2 positions are reserved for layer selectors). Used for the
// button output formula and encoder output numbering.
#define NUM_ACTIVE_BUTTONS (NUMBUTTONS - 2)

// Output numbering:
//   Matrix buttons: 0..(NUM_ACTIVE_BUTTONS*3 - 1)   => 0-68  (23 per layer)
//   Encoders:       ENCODER_BASE..ENCODER_BASE+23   => 69-92 (8 per layer)
// Total used: 93. Declared to the Joystick library as 96 for headroom.
#define ENCODER_BASE (NUM_ACTIVE_BUTTONS * 3)
#define TOTAL_JOYSTICK_BUTTONS 96

#if defined(DIMENSION_3x11)
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4,5,6,7,8,9,10},
  {11,12,13,14,15,16,17,18,19,20,21},
  {22,23,24,25,26,27,28,29,30,31},
};
#elif defined(DIMENSION_6x5)
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23,24},
  {25},
};
#elif defined(DIMENSION_6x4)
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3},
  {4,5,6,7},
  {8,9,10,11},
  {12,13,14,15},
  {16,17,18,19},
  {20,21,22,23},
};
#elif defined(DIMENSION_7x3)
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2},
  {3,4,5},
  {6,7,8},
  {9,10,11},
  {12,13,14},
  {15,16,17},
  {18,19,20},
};
#else
// Covers DIMENSION_5x5 and the default fallback.
// Fixed: last row was previously {20,21,22,23,20} -- duplicate "20" meant
// physical button 24 never worked. Now correctly {20,21,22,23,24}.
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23,24},
};
#endif

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar1; // layer 1 CCW output
  int cwchar1;  // layer 1 CW output
  int ccwchar2; // layer 2 CCW output
  int cwchar2;  // layer 2 CW output
  int ccwchar3; // layer 3 CCW output
  int cwchar3;  // layer 3 CW output
  volatile unsigned char state;
};

// Encoder outputs occupy ENCODER_BASE..ENCODER_BASE+23 (69-92), 8 codes per layer.
rotariesdef rotaries[MAX_ROTARIES] = {
  {0,1, ENCODER_BASE+0,  ENCODER_BASE+1,  ENCODER_BASE+8,  ENCODER_BASE+9,  ENCODER_BASE+16, ENCODER_BASE+17, 0},
  {2,3, ENCODER_BASE+2,  ENCODER_BASE+3,  ENCODER_BASE+10, ENCODER_BASE+11, ENCODER_BASE+18, ENCODER_BASE+19, 0},
  {4,5, ENCODER_BASE+4,  ENCODER_BASE+5,  ENCODER_BASE+12, ENCODER_BASE+13, ENCODER_BASE+20, ENCODER_BASE+21, 0},
  {6,7, ENCODER_BASE+6,  ENCODER_BASE+7,  ENCODER_BASE+14, ENCODER_BASE+15, ENCODER_BASE+22, ENCODER_BASE+23, 0},
};

#define DIR_CCW 0x10
#define DIR_CW 0x20
#define R_START 0x0

#ifdef HALF_STEP
  #define R_CCW_BEGIN 0x1
  #define R_CW_BEGIN 0x2
  #define R_START_M 0x3
  #define R_CW_BEGIN_M 0x4
  #define R_CCW_BEGIN_M 0x5
  const unsigned char ttable[6][4] = {
    // R_START (00)
    {R_START_M, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    // R_CCW_BEGIN
    {R_START_M | DIR_CCW, R_START, R_CCW_BEGIN, R_START},
    // R_CW_BEGIN
    {R_START_M | DIR_CW, R_CW_BEGIN, R_START, R_START},
    // R_START_M (11)
    {R_START_M, R_CCW_BEGIN_M, R_CW_BEGIN_M, R_START},
    // R_CW_BEGIN_M
    {R_START_M, R_START_M, R_CW_BEGIN_M, R_START | DIR_CW},
    // R_CCW_BEGIN_M
    {R_START_M, R_CCW_BEGIN_M, R_START_M, R_START | DIR_CCW},
  };
#else
  #define R_CW_FINAL 0x1
  #define R_CW_BEGIN 0x2
  #define R_CW_NEXT 0x3
  #define R_CCW_BEGIN 0x4
  #define R_CCW_FINAL 0x5
  #define R_CCW_NEXT 0x6
  const unsigned char ttable[7][4] = {
    // R_START
    {R_START, R_CW_BEGIN, R_CCW_BEGIN, R_START},
    // R_CW_FINAL
    {R_CW_NEXT, R_START, R_CW_FINAL, R_START | DIR_CW},
    // R_CW_BEGIN
    {R_CW_NEXT, R_CW_BEGIN, R_START, R_START},
    // R_CW_NEXT
    {R_CW_NEXT, R_CW_BEGIN, R_CW_FINAL, R_START},
    // R_CCW_BEGIN
    {R_CCW_NEXT, R_START, R_CCW_BEGIN, R_START},
    // R_CCW_FINAL
    {R_CCW_NEXT, R_CCW_FINAL, R_START, R_START | DIR_CCW},
    // R_CCW_NEXT
    {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
  };
#endif

#if defined(DIMENSION_3x11)
byte rowPins[NUMROWS] = {21,20,19};
byte colPins[NUMCOLS] = {18,15,16,10,9,8,7,6,5,4,3};
#elif defined(DIMENSION_6x5)
byte rowPins[NUMROWS] = {21,20,19,18,15,14};
byte colPins[NUMCOLS] = {16,10,9,8,7};
#elif defined(DIMENSION_6x4)
byte rowPins[NUMROWS] = {21,20,19,18,15,14};
byte colPins[NUMCOLS] = {16,10,9,8};
#elif defined(DIMENSION_7x3)
byte rowPins[NUMROWS] = {21,20,19,18,15,14,16};
byte colPins[NUMCOLS] = {10,9,8};
#else
// Covers DIMENSION_5x5 and the default fallback. Unchanged from original wiring.
byte rowPins[NUMROWS] = {21,20,19,18,15};
byte colPins[NUMCOLS] = {14,16,10,9,8};
#endif

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS);

Joystick_ Joystick(JOYSTICK_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, TOTAL_JOYSTICK_BUTTONS, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

// Tracks the actual joystick output button each physical key most recently
// pressed, so a release always clears the correct one -- even if the layer
// changes mid-press. 255 = none active.
byte activeOutputButton[NUMBUTTONS];

// Maps each physical kchar (0..NUMBUTTONS-1) to a compact 0..NUM_ACTIVE_BUTTONS-1
// output slot. 255 = this kchar is a layer-select modifier and never outputs.
// Built once in setup() from the EEPROM-loaded selector indices.
byte outputSlotForKchar[NUMBUTTONS];

// Current active layer: 1, 2, or 3. Recomputed once per loop() from the
// live state of the assigned selector buttons (if any).
int currentLayer = 1;

// Runtime-configurable layer selector indices. 255 = unassigned.
// Loaded from EEPROM on boot, modified by long-press programming gestures.
byte layer2Index = 255;
byte layer3Index = 255;

// Programming-mode state machine for dynamic selector assignment.
enum ProgState {
  PROG_IDLE,
  PROG_WAIT_L2,   // Button 1 held, waiting for target
  PROG_WAIT_L3,   // Button 2 held, waiting for target
  PROG_ARMED_L2,  // Button 1 + target held, counting 10s
  PROG_ARMED_L3,  // Button 2 + target held, counting 10s
  PROG_ARMED_RESET // Button 1 + Button 2 held, counting 10s
};
ProgState progState = PROG_IDLE;
byte progTarget = 255;
unsigned long progStartTime = 0;
unsigned long versionStartTime = 0;

void setup() {
  Joystick.begin();
  rotary_init();
  Serial.begin(9600);

  for (int i = 0; i < NUMBUTTONS; i++) {
    activeOutputButton[i] = 255;
  }

  // Load selector indices from EEPROM. Unprogrammed EEPROM (0xFF = 255)
  // naturally defaults to "unassigned". Clamp invalid values.
  layer2Index = EEPROM.read(EEPROM_LAYER2_ADDR);
  layer3Index = EEPROM.read(EEPROM_LAYER3_ADDR);
  if (layer2Index >= NUMBUTTONS) layer2Index = 255;
  if (layer3Index >= NUMBUTTONS) layer3Index = 255;

  // Build the output-slot mapping from the EEPROM-loaded selector indices.
  byte nextSlot = 0;
  for (int kc = 0; kc < NUMBUTTONS; kc++) {
    if (kc == layer2Index || kc == layer3Index) {
      outputSlotForKchar[kc] = 255;
    } else {
      outputSlotForKchar[kc] = nextSlot++;
    }
  }
}

void loop() {
  buttbx.getKeys(); // scan matrix once per loop; all functions below read the result
  UpdateLayer();
  CheckAllEncoders();
  CheckProgrammingGestures();
  CheckAllButtons();
}

// Reads the selector button states and sets currentLayer.
// If no selectors are assigned, remains on Layer 1.
void UpdateLayer() {
  if (layer2Index == 255 && layer3Index == 255) {
    currentLayer = 1;
    return;
  }

  bool layer2Thrown = false;
  bool layer3Thrown = false;

  for (int i=0; i<LIST_MAX; i++) {
    int kchar = buttbx.key[i].kchar;
    KeyState ks = buttbx.key[i].kstate;
    if (kchar == layer2Index && (ks == PRESSED || ks == HOLD)) {
      layer2Thrown = true;
    }
    if (kchar == layer3Index && (ks == PRESSED || ks == HOLD)) {
      layer3Thrown = true;
    }
  }

  if (layer3Thrown) {
    currentLayer = 3;
  } else if (layer2Thrown) {
    currentLayer = 2;
  } else {
    currentLayer = 1;
  }
}

void CheckAllButtons(void) {
  for (int i=0; i<LIST_MAX; i++) {
    if (buttbx.key[i].stateChanged) {
      int kchar = buttbx.key[i].kchar;

      // Layer-select positions never fire a joystick button themselves.
      // Also suppress buttons involved in an active programming gesture.
      if (kchar == layer2Index || kchar == layer3Index || isProgrammingActive(kchar)) {
        continue;
      }

      byte slot = outputSlotForKchar[kchar];

      switch (buttbx.key[i].kstate) {
        case PRESSED: {
#ifdef ROTARY_ONLY_LAYERS
          int outputButton = slot;
#else
          int outputButton = slot + (currentLayer - 1) * NUM_ACTIVE_BUTTONS;
#endif
          activeOutputButton[kchar] = outputButton;
          Joystick.setButton(outputButton, 1);
          break;
        }
        case RELEASED:
        case IDLE: {
          if (activeOutputButton[kchar] != 255) {
            Joystick.setButton(activeOutputButton[kchar], 0);
            activeOutputButton[kchar] = 255;
          }
          break;
        }
      }
    }
  }
}

void rotary_init() {
  for (int i=0;i<NUMROTARIES;i++) {
    pinMode(rotaries[i].pin1, INPUT);
    pinMode(rotaries[i].pin2, INPUT);
    #ifdef ENABLE_PULLUPS
      digitalWrite(rotaries[i].pin1, HIGH);
      digitalWrite(rotaries[i].pin2, HIGH);
    #endif
  }
}

unsigned char rotary_process(int _i) {
  unsigned char pinstate = (digitalRead(rotaries[_i].pin2) << 1) | digitalRead(rotaries[_i].pin1);
  rotaries[_i].state = ttable[rotaries[_i].state & 0xf][pinstate];
  return (rotaries[_i].state & 0x30);
}

void CheckAllEncoders(void) {
  for (int i=0;i<NUMROTARIES;i++) {
    unsigned char result = rotary_process(i);
    int ccw, cw;
    switch (currentLayer) {
      case 2:  ccw = rotaries[i].ccwchar2; cw = rotaries[i].cwchar2; break;
      case 3:  ccw = rotaries[i].ccwchar3; cw = rotaries[i].cwchar3; break;
      default: ccw = rotaries[i].ccwchar1; cw = rotaries[i].cwchar1; break;
    }

    if (result == DIR_CCW) {
      Joystick.setButton(ccw, 1); delay(50); Joystick.setButton(ccw, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(cw, 1); delay(50); Joystick.setButton(cw, 0);
    };
  }
}

// --- Dynamic Layer Selector Assignment ---

// Returns true if kchar is currently involved in a programming gesture
// and should be suppressed (no joystick output).
bool isProgrammingActive(byte kchar) {
  if (progState == PROG_IDLE) return false;
  if ((progState == PROG_ARMED_L2 || progState == PROG_ARMED_L3) && kchar == progTarget) return true;
  return false;
}

// Persists the current selector indices to EEPROM.
void saveSelectorConfig() {
  EEPROM.write(EEPROM_LAYER2_ADDR, layer2Index);
  EEPROM.write(EEPROM_LAYER3_ADDR, layer3Index);
}

// State machine that detects long-press programming gestures:
//   B1 + BX held 10s  -> assign BX as Layer 2 selector
//   B2 + BY held 10s  -> assign BY as Layer 3 selector
//   B1 + B2 held 10s  -> reset both selectors to unassigned
// Early release of any involved button cancels the gesture.
void CheckProgrammingGestures() {
  bool b1Down = false, b2Down = false;
  byte otherDown = 255;

  for (int i = 0; i < LIST_MAX; i++) {
    KeyState ks = buttbx.key[i].kstate;
    if (ks != PRESSED && ks != HOLD) continue;
    int kc = buttbx.key[i].kchar;
    if (kc == 0) b1Down = true;
    else if (kc == 1) b2Down = true;
    else if (otherDown == 255) otherDown = kc;
  }

  switch (progState) {
    case PROG_IDLE:
      if (b1Down && b2Down) {
        progState = PROG_ARMED_RESET;
        progStartTime = millis();
      } else if (b1Down && !b2Down) {
        progState = PROG_WAIT_L2;
      } else if (!b1Down && b2Down) {
        progState = PROG_WAIT_L3;
      } else if (!b1Down && !b2Down && isKeyDown(VERSION_BUTTON)) {
        if (versionStartTime == 0) {
          versionStartTime = millis();
        } else if (millis() - versionStartTime >= PROG_HOLD_DURATION) {
          printConfig();
          versionStartTime = 0;
        }
      } else {
        versionStartTime = 0;
      }
      break;

    case PROG_WAIT_L2:
      if (!b1Down) {
        progState = PROG_IDLE;
      } else if (b2Down) {
        progState = PROG_ARMED_RESET;
        progStartTime = millis();
      } else if (otherDown != 255) {
        progState = PROG_ARMED_L2;
        progTarget = otherDown;
        progStartTime = millis();
        if (activeOutputButton[progTarget] != 255) {
          Joystick.setButton(activeOutputButton[progTarget], 0);
          activeOutputButton[progTarget] = 255;
        }
      }
      break;

    case PROG_WAIT_L3:
      if (!b2Down) {
        progState = PROG_IDLE;
      } else if (b1Down) {
        progState = PROG_ARMED_RESET;
        progStartTime = millis();
      } else if (otherDown != 255) {
        progState = PROG_ARMED_L3;
        progTarget = otherDown;
        progStartTime = millis();
        if (activeOutputButton[progTarget] != 255) {
          Joystick.setButton(activeOutputButton[progTarget], 0);
          activeOutputButton[progTarget] = 255;
        }
      }
      break;

    case PROG_ARMED_L2:
      if (!b1Down || !isKeyDown(progTarget)) {
        progState = PROG_IDLE;
      } else if (millis() - progStartTime >= PROG_HOLD_DURATION) {
        layer2Index = progTarget;
        saveSelectorConfig();
        progState = PROG_IDLE;
        progTarget = 255;
      }
      break;

    case PROG_ARMED_L3:
      if (!b2Down || !isKeyDown(progTarget)) {
        progState = PROG_IDLE;
      } else if (millis() - progStartTime >= PROG_HOLD_DURATION) {
        layer3Index = progTarget;
        saveSelectorConfig();
        progState = PROG_IDLE;
        progTarget = 255;
      }
      break;

    case PROG_ARMED_RESET:
      if (!b1Down || !b2Down) {
        progState = PROG_IDLE;
      } else if (millis() - progStartTime >= PROG_HOLD_DURATION) {
        layer2Index = 255;
        layer3Index = 255;
        saveSelectorConfig();
        progState = PROG_IDLE;
      }
      break;
  }
}

// Prints the current firmware configuration to Serial.
void printConfig() {
  Serial.println(F("--- Button Box Config ---"));
  Serial.print(F("Firmware: v"));
  Serial.println(F(FIRMWARE_VERSION));
  Serial.print(F("Dimension: "));
  Serial.println(F(DIMENSION_STR));
  Serial.print(F("Buttons: "));
  Serial.println(NUMBUTTONS);
  Serial.print(F("Rotaries: "));
  Serial.println(NUMROTARIES);
  Serial.print(F("Controller ID: "));
  Serial.println(CONTROLLER_ID);
  Serial.print(F("Joystick report ID: "));
  Serial.println(JOYSTICK_REPORT_ID);

  Serial.print(F("Layer 2 selector: "));
  if (layer2Index == 255) Serial.println(F("unassigned"));
  else Serial.println(layer2Index);

  Serial.print(F("Layer 3 selector: "));
  if (layer3Index == 255) Serial.println(F("unassigned"));
  else Serial.println(layer3Index);

#ifdef ROTARY_ONLY_LAYERS
  Serial.println(F("Rotary-only layers: true"));
#else
  Serial.println(F("Rotary-only layers: false"));
#endif

  Serial.println(F("--------------------------"));
}

// Helper: returns true if the given kchar is currently pressed or held.
bool isKeyDown(byte kchar) {
  for (int i = 0; i < LIST_MAX; i++) {
    if (buttbx.key[i].kchar == kchar) {
      KeyState ks = buttbx.key[i].kstate;
      if (ks == PRESSED || ks == HOLD) return true;
    }
  }
  return false;
}