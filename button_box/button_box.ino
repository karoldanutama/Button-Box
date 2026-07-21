//BUTTON BOX
//USE w ProMicro
//Tested in WIN10 + Assetto Corsa
//AMSTUDIO
//20.8.17
//
// --- MODIFIED: 3-layer system driven by a single 3-way toggle switch ---
// The toggle's two throw contacts are wired into the button matrix at
// LAYER2_BUTTON_INDEX and LAYER3_BUTTON_INDEX. Because it's a mechanical
// SPDT toggle (not two separate buttons), only one throw can ever be active
// at a time -- the third (center) position leaves both open, which is
// Layer 1 by default. No priority/tie-break logic is needed; the hardware
// itself guarantees mutual exclusivity.
//
// These two positions are silent modifiers: flipping the switch selects a
// layer but never fires a joystick button press itself (a toggle switch
// staying "on" for a whole layer shouldn't also hold a button down the
// entire time).
//
// Fixed a pre-existing bug in the 5x5 button map: the last row repeated
// "20" instead of using "24", meaning physical button 24 never worked.

#include <Keypad.h>
#include <Joystick.h>

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
  #define NUMROWS 3
  #define NUMCOLS 11
  #define NUMROTARIES 0
  #define NUMBUTTONS 32
#elif defined(DIMENSION_6x5)
  #define NUMROWS 6
  #define NUMCOLS 5
  #define NUMROTARIES 3
  #define NUMBUTTONS 26
#elif defined(DIMENSION_6x4)
  #define NUMROWS 6
  #define NUMCOLS 4
  #define NUMROTARIES 4
  #define NUMBUTTONS 24
#elif defined(DIMENSION_7x3)
  #define NUMROWS 7
  #define NUMCOLS 3
  #define NUMROTARIES 4
  #define NUMBUTTONS 21
#elif defined(DIMENSION_5x5)
  #define NUMROWS 5
  #define NUMCOLS 5
  #define NUMROTARIES 4
  #define NUMBUTTONS 25
#else
  #define NUMROWS 5
  #define NUMCOLS 5
  #define NUMROTARIES 4
  #define NUMBUTTONS 25
#endif

// --- LAYER CONFIG ---
// Matrix positions wired to the two throw contacts of the 3-way toggle.
// Neither position ever fires its own joystick button -- see note above.
#define LAYER2_BUTTON_INDEX 8
#define LAYER3_BUTTON_INDEX 7

// Number of matrix buttons that actually produce output: all 25 physical
// positions minus the 2 dedicated to the layer-select toggle.
#define NUM_ACTIVE_BUTTONS (NUMBUTTONS - 2)

// Output numbering:
//   Matrix buttons: 0..(NUM_ACTIVE_BUTTONS*3 - 1)   => 0-68  (23 per layer)
//   Encoders:       ENCODER_BASE..ENCODER_BASE+23   => 69-92 (8 per layer)
// Total used: 93. Declared to the Joystick library as 96 for headroom.
#define ENCODER_BASE (NUM_ACTIVE_BUTTONS * 3)
#define TOTAL_JOYSTICK_BUTTONS 96

#define LONG_PRESS_DURATION 1500 // Hold duration in ms to promote button to layer 2

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

// Timestamp for each button's press, used for long-press-to-layer-2 detection
unsigned long buttonPressTimes[NUMBUTTONS] = {0};

// Tracks the actual joystick output button each physical key most recently
// pressed, so a release always clears the correct one -- even if the layer
// changes mid-press. 255 = none active.
byte activeOutputButton[NUMBUTTONS];

bool buttonLongPressed[NUMBUTTONS];

// Maps each physical kchar (0..NUMBUTTONS-1) to a compact 0..NUM_ACTIVE_BUTTONS-1
// output slot. 255 = this kchar is a layer-select modifier and never outputs.
// Built once in setup() from LAYER2_BUTTON_INDEX / LAYER3_BUTTON_INDEX.
byte outputSlotForKchar[NUMBUTTONS];

// Current active layer: 1, 2, or 3. Recomputed once per loop() from the
// live state of the toggle switch's two throw positions. Since it's a
// mechanical SPDT toggle, only one can ever be active -- this is a plain
// read, not a priority contest.
int currentLayer = 1;

void setup() {
  Joystick.begin();
  rotary_init();

  for (int i = 0; i < NUMBUTTONS; i++) {
    activeOutputButton[i] = 255;
    buttonLongPressed[i] = false;
  }

  // Build the compact output-slot mapping, skipping the two selector kchars.
  byte nextSlot = 0;
  for (int kc = 0; kc < NUMBUTTONS; kc++) {
    if (kc == LAYER2_BUTTON_INDEX || kc == LAYER3_BUTTON_INDEX) {
      outputSlotForKchar[kc] = 255;
    } else {
      outputSlotForKchar[kc] = nextSlot;
      nextSlot++;
    }
  }
  // nextSlot should now equal NUM_ACTIVE_BUTTONS.
}

void loop() {
  buttbx.getKeys(); // scan matrix once per loop; both functions below read the result
  UpdateLayer();
  CheckAllEncoders();
  CheckAllButtons();
}

// Reads the toggle switch's two throw positions and sets currentLayer.
// Mechanically exclusive -- both can never read HOLD/PRESSED at once --
// but the else-chain below is written defensively regardless.
void UpdateLayer() {
  bool layer2Thrown = false;
  bool layer3Thrown = false;

  for (int i=0; i<LIST_MAX; i++) {
    int kchar = buttbx.key[i].kchar;
    KeyState ks = buttbx.key[i].kstate;
    if (kchar == LAYER2_BUTTON_INDEX && (ks == PRESSED || ks == HOLD)) {
      layer2Thrown = true;
    }
    if (kchar == LAYER3_BUTTON_INDEX && (ks == PRESSED || ks == HOLD)) {
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
      if (kchar == LAYER2_BUTTON_INDEX || kchar == LAYER3_BUTTON_INDEX) {
        continue;
      }

      byte slot = outputSlotForKchar[kchar];

      switch (buttbx.key[i].kstate) {
        case PRESSED: {
          // Record the time when button is pressed
          buttonPressTimes[kchar] = millis();
          buttonLongPressed[kchar] = false;
          int outputButton = slot + (currentLayer - 1) * NUM_ACTIVE_BUTTONS;
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
          buttonLongPressed[kchar] = false;
          break;
        }
      }
    }
  }

  // Check held buttons for long-press promotion to layer 2
  for (int i=0; i<LIST_MAX; i++) {
    int kchar = buttbx.key[i].kchar;
    if (kchar == LAYER2_BUTTON_INDEX || kchar == LAYER3_BUTTON_INDEX) continue;

    switch (buttbx.key[i].kstate) {
      case HOLD:
        if (currentLayer != 2 && !buttonLongPressed[kchar]) {
          if (millis() - buttonPressTimes[kchar] >= LONG_PRESS_DURATION) {
            if (activeOutputButton[kchar] != 255) {
              Joystick.setButton(activeOutputButton[kchar], 0);
            }
            int layer2Button = outputSlotForKchar[kchar] + (2 - 1) * NUM_ACTIVE_BUTTONS;
            activeOutputButton[kchar] = layer2Button;
            Joystick.setButton(layer2Button, 1);
            buttonLongPressed[kchar] = true;
          }
        }
        break;
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