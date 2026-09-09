#include "classes/ReleaseDebounce.h"

// --------------------------------------------------- Macros ---------------------------------------------------

#define BUTTON_PIN 2

// --------------------------------------------------------------------------------------------------------------

//debounce classes done by chatgpt lol

ReleaseDebounce::ReleaseDebounce(uint8_t buttonPin) {
  pin = buttonPin;
  pressedFlag = false;
  pressStartTime = 0;
  lastHoldDuration = 0;
}

void ReleaseDebounce::begin() {
  pinMode(pin, INPUT_PULLUP);
}

bool ReleaseDebounce::update() {
  bool state = digitalRead(pin);

  if (state == LOW && !pressedFlag) {
    // button pressed down, but don't trigger yet
    pressedFlag = true;
    pressStartTime = millis();
  }

  if (state == HIGH && pressedFlag) {
    // button released -> trigger event
    lastHoldDuration = millis() - pressStartTime;
    pressedFlag = false;
    return true;
  }

  return false; // no event
}

bool ReleaseDebounce::isPressed() {
  return digitalRead(pin) == LOW;
}

unsigned long ReleaseDebounce::getHoldDuration() const {
  if (pressedFlag) {
    return millis() - pressStartTime;
  }

  return lastHoldDuration;
}

// ----------------------------------------------------- Objects -----------------------------------------------------

ReleaseDebounce selection_button(BUTTON_PIN);
