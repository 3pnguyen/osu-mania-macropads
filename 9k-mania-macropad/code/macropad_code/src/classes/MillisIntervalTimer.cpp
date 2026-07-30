#include "classes/MillisIntervalTimer.h"

MillisIntervalTimer::MillisIntervalTimer(unsigned long intervalMs) : interval(intervalMs) {}

bool MillisIntervalTimer::isReady() {
  unsigned long currentTime = millis();

  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    return true;
  }
  return false;
}

void MillisIntervalTimer::reset() {
  lastTime = millis();
}

// ----------------------------------------------------- Objects -----------------------------------------------------

MillisIntervalTimer statusCheck(2000);
