#pragma once

#include <Arduino.h>

class MillisIntervalTimer {
  private:
    unsigned long lastTime = 0;
    unsigned long interval;

  public:
    MillisIntervalTimer(unsigned long intervalMs);
    bool isReady();
    void reset();
};

// ----------------------------------------------------- Objects -----------------------------------------------------

extern MillisIntervalTimer statusCheck;
