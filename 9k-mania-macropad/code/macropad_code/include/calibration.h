#pragma once

#include <Arduino.h>
#include <ADC.h>
#include <EEPROM.h>

struct KeyCalibrationProfile {
    int adc_released;
    int adc_pressed;
};

inline constexpr int eeprom_start_address = 0;
inline constexpr int adc_max_value = 4095;

inline void loadCalibration(KeyCalibrationProfile *keyProfiles, int totalKeys) {
    int address = eeprom_start_address;

    for (int i = 0; i < totalKeys; i++) {
        EEPROM.get(address, keyProfiles[i]);
        address += sizeof(KeyCalibrationProfile);
    }
}

inline void saveCalibration(const KeyCalibrationProfile *keyProfiles, int totalKeys) {
    int address = eeprom_start_address;

    for (int i = 0; i < totalKeys; i++) {
        EEPROM.put(address, keyProfiles[i]);
        address += sizeof(KeyCalibrationProfile);
    }
}

inline bool checkCalibration(const KeyCalibrationProfile *keyProfiles, int totalKeys, bool invert_adc) {
    bool needsCalibration = false;

    for (int i = 0; i < totalKeys; i++) {
        const bool valuesOutOfRange =
            keyProfiles[i].adc_released < 0 ||
            keyProfiles[i].adc_released > adc_max_value ||
            keyProfiles[i].adc_pressed < 0 ||
            keyProfiles[i].adc_pressed > adc_max_value;
        const bool directionInvalid = invert_adc
            ? keyProfiles[i].adc_pressed >= keyProfiles[i].adc_released
            : keyProfiles[i].adc_pressed <= keyProfiles[i].adc_released;

        if (valuesOutOfRange || directionInvalid) {
            Serial.print("Invalid calibration data for key ");
            Serial.println(i);
            needsCalibration = true;
        }
    }

    if (needsCalibration) {
        Serial.println("No valid calibration profile found in memory.");
        Serial.println("Hold the selection button to run calibration.");
        return false;
    } else {
        Serial.println("Calibration data successfully loaded from EEPROM.");

        for (int i = 0 ; i < totalKeys; i++) {
            Serial.print("Key ");
            Serial.print(i);
            Serial.print(" released: ");
            Serial.print(keyProfiles[i].adc_released);
            Serial.print(" pressed: ");
            Serial.println(keyProfiles[i].adc_pressed);
        }
    }

    return true;
}

inline void runCalibration(ADC *adc, int *switchPins, KeyCalibrationProfile *keyProfiles, int totalKeys, int ledPin, int ledBrightness, bool invert_adc) {
    analogWrite(ledPin, ledBrightness);
    Serial.println("\n==================================== Hall Effect Calibration Mode ====================================");



    // First, capture the resting values for each key -----------------------------------------------------------------
    Serial.println("\nFirst part: capturing resting values.");
    Serial.println("Ensure your hands are completely off the keyboard.");
    Serial.println("Type 'Y' and press Enter to capture resting values...");

    while (!Serial.available()) { delay(10); }
    Serial.readStringUntil('\n'); // Clear the buffer

    for (int i = 3; i > 0; i--) { // Count down
        Serial.print(i);
        Serial.println("...");
        delay(1000);
    }

    Serial.println("\nCapturing baseline resting values... ");

    for (int i = 0; i < totalKeys; i++) {
        keyProfiles[i].adc_released = adc->adc0->analogRead(switchPins[i]);
    }

    Serial.println("DONE!");



    // Second, capture the fully pressed values for each key -----------------------------------------------------------------
    Serial.println("\nNext step: Bottom out travel mapping.");
    Serial.println("Slowly and firmly press down EVERY key all the way to the bottom.");
    Serial.println("Keep pressing them completely until instructed.");
    Serial.println("Type 'Y' and press Enter when you are ready to begin scanning...");

    while (!Serial.available()) { delay(10); }
    Serial.readStringUntil('\n'); // Clear the buffer

    for (int i = 3; i > 0; i--) { // Count down
        Serial.print(i);
        Serial.println("...");
        delay(1000);
    }

    Serial.print("\nCapturing fully pressed values... ");
    Serial.println("Scanning will last for 5 seconds...");

    unsigned long startTime = millis();

    // Initialize the pressed endpoints with the released readings before tracking travel.
    for (int i = 0; i < totalKeys; i++) {
        keyProfiles[i].adc_pressed = keyProfiles[i].adc_released;
    }

    // Actively track peak values while the user holds/mashes the keys
    while (millis() - startTime < 5000) {
        for (int i = 0; i < totalKeys; i++) {
            uint16_t liveAdc = adc->adc0->analogRead(switchPins[i]);
            
            // Track the endpoint in the configured direction.
            if ((!invert_adc && liveAdc > keyProfiles[i].adc_pressed) ||
                (invert_adc && liveAdc < keyProfiles[i].adc_pressed)) {
                keyProfiles[i].adc_pressed = liveAdc;
            }
        }
        delay(5); // Small delay to avoid overloading the processor loop
    }

    Serial.println("Scanning complete! You can release the keys.");



    // Finally, write the calibration data to EEPROM -----------------------------------------------------------------
    Serial.println("\nWriting calibration data to EEPROM... ");
    saveCalibration(keyProfiles, totalKeys);

    Serial.println("DONE!");
    Serial.println("Calibration profile locked in. Rebooting baseline logic.\n");
    digitalWrite(ledPin, LOW);
}
