#include <Arduino.h>
#include <Keyboard.h>

// --------------------------------------------------- Macros ---------------------------------------------------

#define SW1_PIN 2
#define SW2_PIN 3
#define SW3_PIN 4
#define SW4_PIN 5

#define SW1_KEY 'z'
#define SW2_KEY 'x'
#define SW3_KEY '.'
#define SW4_KEY '/'

#define DEBOUNCE_DELAY 3

// --------------------------------------------------------------------------------------------------------------

int keyPins[] = {SW1_PIN, SW2_PIN, SW3_PIN, SW4_PIN};
bool keyPreviousStates[] = {false, false, false, false};
unsigned long keyDebounceStartTimes[] = {0, 0, 0, 0};
char keys[] = {SW1_KEY, SW2_KEY, SW3_KEY, SW4_KEY};

// a function that will press and release a key with debounce
void checkKey(int pin, bool& previousState, unsigned long& debounceStartTime, char key);

void setup() {
  for (int i = 0; i < 4; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }

  Keyboard.begin();
  delay(100);
}

void loop() {
  for (int i = 0; i < 4; i++) {
    checkKey(SW1_PIN + i, keyPreviousStates[i], keyDebounceStartTimes[i], keys[i]);
  }
}

void checkKey (int pin, bool& previousState, unsigned long& debounceStartTime, char key) {
  int currentState = digitalRead(pin);

  if (currentState != previousState) { // if the state has changed then reset the debounce timer and update the previous state
    debounceStartTime = millis();
    previousState = currentState;
  }

  if (millis() - debounceStartTime > DEBOUNCE_DELAY) { // if the debounce timer has expired then check the current state
    if (currentState == LOW) {
      Keyboard.press(key);
    } else {
      Keyboard.release(key);
    }
  }
}
