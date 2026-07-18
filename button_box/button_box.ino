//BUTTON BOX
//USE w ProMicro
//Tested in WIN10 + Assetto Corsa
//AMSTUDIO
//20.8.17
//
// --- MODIFIED: firmware-native shift/chording layer added ---
// One physical button acts as a hardware "shift" key. While held, every other
// button AND all 4 rotary encoders output a second, distinct set of joystick
// button numbers. This replaces AntiMicroX's Sets feature entirely -- the
// layer logic now lives on the microcontroller, so it works the instant you
// plug in, with no PC-side software running.

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
#else
  #define NUMROWS 5
  #define NUMCOLS 5
  #define NUMROTARIES 4
  #define NUMBUTTONS 25
#endif

// --- SHIFT LAYER CONFIG ---
// Physical matrix button that acts as the shift/modifier key.
// Default: button 23 = bottom-right button of the 6x4 matrix.
// Change this if you'd rather use a different physical button.
#define SHIFT_BUTTON_INDEX 8

// Layer offset: unshifted matrix buttons output 0..NUMBUTTONS-1 as normal.
// Shifted matrix buttons output (kchar + LAYER_OFFSET) instead.
#define LAYER_OFFSET 24

// Total joystick buttons needed: 24 (layer 1) + 24 (layer 2) + 8 encoder
// codes (layer 1) + 8 encoder codes (layer 2) = 64. Well within the
// Joystick library's tested range of up to 128.
#define TOTAL_JOYSTICK_BUTTONS 64

// EEPROM address for storing button press duration
#define EEPROM_BUTTON_DURATION_ADDR 0

// Button assignments
#define TOGGLE_BUTTON_DURATION_BTN 100 /// Disable this function for now
#define TOGGLE_HOLD_DURATION 3000 // Hold duration in ms for toggling button mode
#define DEFAULT_PRESS_HOLD -1

// Initial button press duration (will be overridden by EEPROM value if it exists)
int buttonPressDuration = DEFAULT_PRESS_HOLD; // -1 for original behavior, positive value for momentary press duration in ms

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
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23,20},
};
#endif

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;      // layer 1 (unshifted) CCW output
  int cwchar;       // layer 1 (unshifted) CW output
  int ccwchar2;      // layer 2 (shifted) CCW output
  int cwchar2;       // layer 2 (shifted) CW output
  volatile unsigned char state;
};

// Renumbered so encoder outputs (48-63) never collide with matrix button
// outputs (0-47, covering both layers of the 24 physical buttons).
rotariesdef rotaries[MAX_ROTARIES] = {
  {0,1, 48,49, 56,57, 0},
  {2,3, 50,51, 58,59, 0},
  {4,5, 52,53, 60,61, 0},
  {6,7, 54,55, 62,63, 0},
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
byte rowPins[NUMROWS] = {21,20,19,18,15};
byte colPins[NUMCOLS] = {14,16,10,9,8};
#endif

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS);

Joystick_ Joystick(JOYSTICK_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, TOTAL_JOYSTICK_BUTTONS, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

// Add timestamp array for button press tracking
unsigned long buttonPressTimes[NUMBUTTONS] = {0};

// Tracks the actual joystick output button each physical key most recently
// pressed, so a release always clears the correct one -- even if the shift
// key is pressed or released mid-press on another button. 255 = none active.
byte activeOutputButton[NUMBUTTONS];

// True while the shift button is physically held down.
bool shiftActive = false;

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

void setup() {
  Joystick.begin();
  rotary_init();
  pinMode(LED_BUILTIN, OUTPUT);

  for (int i = 0; i < NUMBUTTONS; i++) {
    activeOutputButton[i] = 255;
  }

  // Read button press duration from EEPROM
  int storedDuration = EEPROM.read(EEPROM_BUTTON_DURATION_ADDR);
  if (storedDuration != 255) { // 255 is the default uninitialized EEPROM value
    buttonPressDuration = storedDuration;
  }
}

void loop() {
  CheckAllEncoders();
  CheckAllButtons();
}

void CheckAllButtons(void) {
  if (buttbx.getKeys()) {
    for (int i=0; i<LIST_MAX; i++) {
      if (buttbx.key[i].stateChanged) {
        int kchar = buttbx.key[i].kchar;

        // The shift button itself never sends a joystick press -- it only
        // toggles which layer everything else maps to.
        if (kchar == SHIFT_BUTTON_INDEX) {
          switch (buttbx.key[i].kstate) {
            case PRESSED:
            case HOLD:
              shiftActive = true;
              break;
            case RELEASED:
            case IDLE:
              shiftActive = false;
              break;
          }
          continue;
        }

        switch (buttbx.key[i].kstate) {
          case PRESSED: {
            // Record the time when button is pressed
            buttonPressTimes[kchar] = millis();
            int outputButton = shiftActive ? (kchar + LAYER_OFFSET) : kchar;
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

  // Check for auto-release on all pressed buttons
  for (int i=0; i<LIST_MAX; i++) {
    int kchar = buttbx.key[i].kchar;
    if (kchar == SHIFT_BUTTON_INDEX) continue;

    switch (buttbx.key[i].kstate) {
      case HOLD:
        if (kchar == TOGGLE_BUTTON_DURATION_BTN) {
          if (millis() - buttonPressTimes[kchar] >= TOGGLE_HOLD_DURATION) {
            // Toggle between -1 and 50
            buttonPressDuration = (buttonPressDuration == -1) ? DEFAULT_PRESS_HOLD : -1;
            // Save to EEPROM
            EEPROM.write(EEPROM_BUTTON_DURATION_ADDR, buttonPressDuration);
            // Blink LED three times to indicate change
            blinkLED(3);
            // Reset the press time to prevent multiple toggles
            buttonPressTimes[kchar] = millis();
          }
        } else if (buttonPressDuration > 0) {
          if (millis() - buttonPressTimes[kchar] >= buttonPressDuration) {
            if (activeOutputButton[kchar] != 255) {
              Joystick.setButton(activeOutputButton[kchar], 0);
              activeOutputButton[kchar] = 255;
            }
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
    int ccw = shiftActive ? rotaries[i].ccwchar2 : rotaries[i].ccwchar;
    int cw  = shiftActive ? rotaries[i].cwchar2  : rotaries[i].cwchar;

    if (result == DIR_CCW) {
      Joystick.setButton(ccw, 1); delay(50); Joystick.setButton(ccw, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(cw, 1); delay(50); Joystick.setButton(cw, 0);
    };
  }
}
