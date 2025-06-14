//BUTTON BOX 
//USE w ProMicro
//Tested in WIN10 + Assetto Corsa
//AMSTUDIO
//20.8.17

#include <Keypad.h>
#include <Joystick.h>
#include <EEPROM.h>

// Controller Configuration
#define CONTROLLER_ID 2  // Change this to 1 for the second controller, 2 for third, etc.

#define DIMENSION_3x11
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
#else
#define NUMROWS 5
#define NUMCOLS 5
#define NUMROTARIES 4
#define NUMBUTTONS 25
#endif

// EEPROM address for storing button press duration
#define EEPROM_BUTTON_DURATION_ADDR 0

// Button assignments
#define TOGGLE_BUTTON_DURATION_BTN 18
#define TOGGLE_HOLD_DURATION 3000  // Hold duration in ms for toggling button mode


#define DEFAULT_PRESS_HOLD 50
// Initial button press duration (will be overridden by EEPROM value if it exists)
int buttonPressDuration = DEFAULT_PRESS_HOLD;  // -1 for original behavior, positive value for momentary press duration in ms

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
#else
byte buttons[NUMROWS][NUMCOLS] = {
  {0,1,2,3,4},
  {5,6,7,8,9},
  {10,11,12,13,14},
  {15,16,17,18,19},
  {20,21,22,23},
};
#endif

struct rotariesdef {
  byte pin1;
  byte pin2;
  int ccwchar;
  int cwchar;
  volatile unsigned char state;
};

rotariesdef rotaries[MAX_ROTARIES] = {
  {0,1,24,25,0},
  {2,3,26,27,0},
  {4,5,28,29,0},
  {6,7,30,31,0},
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
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
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
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
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
#else
byte rowPins[NUMROWS] = {21,20,19,18,15}; 
byte colPins[NUMCOLS] = {14,16,10,9,8}; 
#endif

Keypad buttbx = Keypad( makeKeymap(buttons), rowPins, colPins, NUMROWS, NUMCOLS); 

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID + CONTROLLER_ID, 
  JOYSTICK_TYPE_JOYSTICK, 32, 0,
  false, false, false, false, false, false,
  false, false, false, false, false);

// Add timestamp array for button press tracking
unsigned long buttonPressTimes[NUMBUTTONS] = {0};

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
        switch (buttbx.key[i].kstate) {
          case PRESSED:
            // Record the time when button is pressed
            buttonPressTimes[buttbx.key[i].kchar] = millis();
            Joystick.setButton(buttbx.key[i].kchar, 1);
            break;
          case RELEASED:
          case IDLE:
            Joystick.setButton(buttbx.key[i].kchar, 0);
            break;
        }
      }
    }
  }

  // Check for auto-release on all pressed buttons
  for (int i=0; i<LIST_MAX; i++) {
    switch (buttbx.key[i].kstate) {
      case HOLD:
        if (buttbx.key[i].kchar == TOGGLE_BUTTON_DURATION_BTN) {
          if (millis() - buttonPressTimes[buttbx.key[i].kchar] >= TOGGLE_HOLD_DURATION) {
            // Toggle between -1 and 50
            buttonPressDuration = (buttonPressDuration == -1) ? DEFAULT_PRESS_HOLD : -1;
            // Save to EEPROM
            EEPROM.write(EEPROM_BUTTON_DURATION_ADDR, buttonPressDuration);
            // Blink LED three times to indicate change
            blinkLED(3);
            // Reset the press time to prevent multiple toggles
            buttonPressTimes[buttbx.key[i].kchar] = millis();
          }
        } else if (buttonPressDuration > 0) {
          if (millis() - buttonPressTimes[buttbx.key[i].kchar] >= buttonPressDuration) {
            Joystick.setButton(buttbx.key[i].kchar, 0);
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
    if (result == DIR_CCW) {
      Joystick.setButton(rotaries[i].ccwchar, 1); delay(50); Joystick.setButton(rotaries[i].ccwchar, 0);
    };
    if (result == DIR_CW) {
      Joystick.setButton(rotaries[i].cwchar, 1); delay(50); Joystick.setButton(rotaries[i].cwchar, 0);
    };
  }
}
