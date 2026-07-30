#pragma once

#include <Arduino.h>

class ReleaseDebounce {
  private:
    uint8_t pin;
    bool pressedFlag;
    unsigned long pressStartTime;
    unsigned long lastHoldDuration;

  public:
    ReleaseDebounce(uint8_t buttonPin);
    bool update();
    bool isPressed();
    unsigned long getHoldDuration() const;
};

// ----------------------------------------------------- Objects -----------------------------------------------------

extern ReleaseDebounce selection_button;
