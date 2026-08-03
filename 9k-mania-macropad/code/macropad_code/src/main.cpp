#include <Arduino.h>
#include <ADC.h>
#include <EEPROM.h>
#include <Keyboard.h>

#include "calibration.h"
#include "calculations.h"
#include "keybinds.h"

#include "classes/ReleaseDebounce.h"
#include "classes/MillisIntervalTimer.h"

// --------------------------------------------------- Macros ---------------------------------------------------

#define FORCE_TEENSY_TO_WAIT_FOR_SERIAL 0

#define ACTUATION_MM 2.0f
#define TOP_DEADBAND_MM 0.15
#define BOTTOM_DEADBAND_MM 0.15
#define RT_PRESS_SENSITIVITY 0.15f 
#define RT_RELEASE_SENSITIVITY 0.15f
#define INVERT_ADC_READINGS true

#define LED_PIN 3
#define LED_BRIGHTNESS (int)(0.5 * 255)
#define LED_DELAY 150

// --------------------------------------------------------------------------------------------------------------

const int total_sets = 2;

SettingsProfile settings{};
int selection = 0;
KeyCalibrationProfile keyProfiles[total_keys];
RapidTriggerProfile rapidTriggerProfiles[total_keys];
int switchPins[total_keys] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9};
ADC *adc = new ADC();
bool calibrationValid = false;

void blinkLED(int pin, int brightness, int cycles, int delay_ms) {
  for (int i = 0; i < cycles; i++) {
        analogWrite(pin, brightness);
        delay(delay_ms);
        digitalWrite(pin, LOW);
        delay(delay_ms);
  }
};

void setup() {
  Serial.begin(115200);
  #if FORCE_TEENSY_TO_WAIT_FOR_SERIAL
    while (!Serial) { delay(10); }
  #endif

  Keyboard.begin();

  setupCalculations(ACTUATION_MM, TOP_DEADBAND_MM, BOTTOM_DEADBAND_MM, RT_PRESS_SENSITIVITY, RT_RELEASE_SENSITIVITY);

  for (int i = 0; i < 10; i++) {
    pinMode(switchPins[i], INPUT);
  }

  adc->adc0->setAveraging(16); // Setup ADC
  adc->adc0->setResolution(12);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);

  loadCalibration(keyProfiles, total_keys); // Load calibration data
  calibrationValid = checkCalibration(keyProfiles, total_keys, INVERT_ADC_READINGS); // Check if calibration data is valid

  blinkLED(LED_PIN, LED_BRIGHTNESS, 1, LED_DELAY);
  delay(100);
}

void loop() {

  if (selection_button.update()) {
    unsigned long holdDuration = selection_button.getHoldDuration();

    if (holdDuration > 500) {
      runCalibration(adc, switchPins, keyProfiles, total_keys, LED_PIN, LED_BRIGHTNESS, INVERT_ADC_READINGS);
      calibrationValid = checkCalibration(keyProfiles, total_keys, INVERT_ADC_READINGS);
    } else {
      selection = (selection + 1) % total_sets;
      Serial.print("Selection = ");
      Serial.println(selection);
      blinkLED(LED_PIN, LED_BRIGHTNESS, selection + 1, LED_DELAY);
    }
  }

  if (!calibrationValid) {
    delay(10);
    return;
  }

  for (int i = 0; i < total_keys; i++) {
    int adc_live = adc->adc0->analogRead(switchPins[i]); // Get normalized ADC value
    float distance_mm = getDistanceMM(adc_live, keyProfiles[i].adc_released, keyProfiles[i].adc_pressed, INVERT_ADC_READINGS); // Convert normalized ADC value to distance
    const KeyCommand& command = (selection == 0) ? switchKeysSetOne[i] : switchKeysSetTwo[i];
    
    if (command.type == CommandType::Key) {
      isKeyPressed(distance_mm, &rapidTriggerProfiles[i], command.key);
    } else if (command.type == CommandType::Text) {
      isKeyPressed(distance_mm, &rapidTriggerProfiles[i], command.text);
    }
  }

  if (statusCheck.isReady()) { 
    statusCheck.reset(); // shows that the serial communication is working without having to block the main loop via !serial in setup()
    Serial.println("Working... ");
  }

}
