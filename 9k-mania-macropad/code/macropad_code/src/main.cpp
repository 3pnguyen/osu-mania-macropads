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
#define INVERT_ADC_READINGS false
#define LUT_SIZE 17

#define ACTUATION_MM 2.0f
#define TOP_DEADBAND_MM 0.15f
#define BOTTOM_DEADBAND_MM 0.15f
#define RT_PRESS_SENSITIVITY 0.15f 
#define RT_RELEASE_SENSITIVITY 0.15f

#define LED_PIN 3
#define LED_BRIGHTNESS (int)(0.5 * 255)
#define LED_DELAY 150

// --------------------------------------------------------------------------------------------------------------

const int total_sets = 2;
int selection = 0;
ADC *adc = new ADC();

int switchPins[total_keys] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9};
bool calibrationPerSwitchValid[total_keys] = {true, true, true, true, true, true, true, true, true, true};
SettingsProfile settings{};
KeyCalibrationProfile keyProfiles[total_keys];
RapidTriggerProfile rapidTriggerProfiles[total_keys];
float switchDistances[total_keys] = {3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5};
SwitchProfile switchProfiles[total_keys] = { 
  {switchDistances[0], createLUT(LUT_SIZE, switchDistances[0])},
  {switchDistances[1], createLUT(LUT_SIZE, switchDistances[1])},
  {switchDistances[2], createLUT(LUT_SIZE, switchDistances[2])},
  {switchDistances[3], createLUT(LUT_SIZE, switchDistances[3])},
  {switchDistances[4], createLUT(LUT_SIZE, switchDistances[4])},
  {switchDistances[5], createLUT(LUT_SIZE, switchDistances[5])},
  {switchDistances[6], createLUT(LUT_SIZE, switchDistances[6])},
  {switchDistances[7], createLUT(LUT_SIZE, switchDistances[7])},
  {switchDistances[8], createLUT(LUT_SIZE, switchDistances[8])},
  {switchDistances[9], createLUT(LUT_SIZE, switchDistances[9])}
};

void blinkLED(int pin, int brightness, int cycles, int delay_ms) {
  for (int i = 0; i < cycles; i++) {
        analogWrite(pin, brightness);
        delay(delay_ms);
        analogWrite(pin, 0);
        delay(delay_ms);
  }
};

void setup() {
  Serial.begin(115200);
  #if FORCE_TEENSY_TO_WAIT_FOR_SERIAL
    while (!Serial) { delay(10); }
  #endif

  Keyboard.begin();
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);
  selection_button.begin();

  setupCalculations(ACTUATION_MM, TOP_DEADBAND_MM, BOTTOM_DEADBAND_MM, RT_PRESS_SENSITIVITY, RT_RELEASE_SENSITIVITY);

  for (int i = 0; i < 10; i++) {
    pinMode(switchPins[i], INPUT);
  }

  adc->adc0->setAveraging(16); // Setup ADC
  adc->adc0->setResolution(12);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);

  loadCalibration(keyProfiles, total_keys); // Load calibration data

  Serial.println();
  for (int i = 0; i < 10; i++) {
    calibrationPerSwitchValid[i] = checkCalibrationIndividual(keyProfiles[i], INVERT_ADC_READINGS);
    if (calibrationPerSwitchValid[i] == true) {
      Serial.print("Valid calibration -> ");
      Serial.print("Key ");
      Serial.print(i);
      Serial.print(" released: ");
      Serial.print(keyProfiles[i].adc_released);
      Serial.print(" pressed: ");
      Serial.println(keyProfiles[i].adc_pressed);
    } else {
      Serial.print("Invalid calibration for key ");
      Serial.println(i);
    }
  }

  blinkLED(LED_PIN, LED_BRIGHTNESS, 1, LED_DELAY);
  delay(100);
}

void loop() {

  if (selection_button.update()) {
    unsigned long holdDuration = selection_button.getHoldDuration();

    if (holdDuration > 3000) {
      runCalibration(adc, switchPins, keyProfiles, total_keys, LED_PIN, LED_BRIGHTNESS, INVERT_ADC_READINGS);
      Serial.println();
      for (int i = 0; i < 10; i++) {
        calibrationPerSwitchValid[i] = checkCalibrationIndividual(keyProfiles[i], INVERT_ADC_READINGS);
        if (calibrationPerSwitchValid[i] == true) {
          Serial.print("Valid calibration -> ");
          Serial.print("Key ");
          Serial.print(i);
          Serial.print(" released: ");
          Serial.print(keyProfiles[i].adc_released);
          Serial.print(" pressed: ");
          Serial.println(keyProfiles[i].adc_pressed);
        } else {
          Serial.print("Invalid calibration for key ");
          Serial.println(i);
        }
      }
    } else {
      selection = (selection + 1) % total_sets;
      Serial.println();
      Serial.print("Selection = ");
      Serial.println(selection);
      blinkLED(LED_PIN, LED_BRIGHTNESS, selection + 1, LED_DELAY);
    }
  }

  Serial.print("\rLive switch presses: ");

  for (int i = 0; i < total_keys; i++) {
    int adc_live = adc->adc0->analogRead(switchPins[i]); // Get normalized ADC value
    float distance_mm = getDistanceMM(adc_live, keyProfiles[i].adc_released, keyProfiles[i].adc_pressed, &switchProfiles[i],  INVERT_ADC_READINGS); // Convert normalized ADC value to distance
    const KeyCommand& command = (selection == 0) ? switchKeysSetOne[i] : switchKeysSetTwo[i];

    if (calibrationPerSwitchValid[i] == true) {
      if (command.type == CommandType::Key) {
        isKeyPressed(distance_mm, &rapidTriggerProfiles[i], command.key);
      } else if (command.type == CommandType::Text) {
        isKeyPressed(distance_mm, &rapidTriggerProfiles[i], command.text);
      }
    }

    Serial.print(rapidTriggerProfiles[i].is_pressed ? "pressed (" : "- (");
    Serial.print(distance_mm);
    Serial.print(")");
    if (i != total_keys - 1) Serial.print(", ");
  }
  
  Serial.print("        ");
}
