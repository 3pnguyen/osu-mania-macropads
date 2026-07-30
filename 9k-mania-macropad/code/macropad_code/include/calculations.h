#pragma once

#include <Keyboard.h>

struct SettingsProfile {
    float actuation_mm;
    float top_deadband_mm;
    float bottom_deadband_mm;
    float rt_press_sensitivity;
    float rt_release_sensitivity;
};

struct RapidTriggerProfile {
    float current_mm;
    float min_bound_mm;
    float max_bound_mm;
    bool is_pressed;
};

extern SettingsProfile settings;
extern const float total_travel_mm = 3.2f;
extern const int lut_size = 17;
extern const float LUT[lut_size] = { // lookup table to convert ADC value to distance
    0.00f, 0.48f, 0.91f, 1.30f, 1.64f, 1.95f, 2.21f, 2.44f, 2.63f, 2.79f, 2.92f, 3.03f, 3.10f, 3.15f, 3.18f, 3.20f, 3.20f
};

inline void setupCalculations(float actuation_mm, float top_deadband_mm, float bottom_deadband_mm, float rt_press_sensitivity, float rt_release_sensitivity) { // setup calculations
    settings.actuation_mm = actuation_mm;
    settings.top_deadband_mm = top_deadband_mm;
    settings.bottom_deadband_mm = bottom_deadband_mm;
    settings.rt_press_sensitivity = rt_press_sensitivity;
    settings.rt_release_sensitivity = rt_release_sensitivity;
}

inline float normalizeADC(int adc_live, int adc_min, int adc_max) { // normalize ADC value to 0.0 to 1.0
    if (adc_live < adc_min) adc_live = adc_min; // clamp
    if (adc_live > adc_max) adc_live = adc_max;
    
    return (float)(adc_live - adc_min) / (adc_max - adc_min); 
}

inline float getDistanceMM(int adc_live, int adc_min, int adc_max) { // convert normalized ADC value to distance
    float normalized_adc = normalizeADC(adc_live, adc_min, adc_max);
    
    if (normalized_adc <= 0.0f) return 0.0f; // clamp
    if (normalized_adc >= 1.0f) return total_travel_mm;
    
    float table_position = normalized_adc * (lut_size - 1); // get position in lookup table

    int low_index = (int)table_position;
    int high_index = low_index + 1;
    float blend = table_position - (float)low_index;

    // Look up the two surrounding linear percentage values
    float y0 = LUT[low_index];
    float y1 = LUT[high_index];

    // Perform the standard linear interpolation formula: y = y0 + blend * (y1 - y0)
    float linearized_pct = y0 + blend * (y1 - y0);

    // Convert the finalized percentage to physical millimeters
    float travel_distance = linearized_pct * total_travel_mm;

    // Apply deadbands to the top and bottom of the switch
    if (travel_distance < settings.top_deadband_mm) return 0.0f;
    if (travel_distance > (total_travel_mm - settings.bottom_deadband_mm)) return total_travel_mm;
    
    return travel_distance;
}

inline void isKeyPressed(float distance_mm, RapidTriggerProfile *key_profile, uint16_t key) { // rapid trigger under the actuation point
    key_profile->current_mm = distance_mm;

    if (!key_profile->is_pressed) {
        // Track the highest point reached while released. Once the switch has
        // crossed the fixed actuation point, downward motion can re-actuate it.
        if (key_profile->current_mm < key_profile->min_bound_mm) {
            key_profile->min_bound_mm = key_profile->current_mm;
        }

        if (key_profile->current_mm >= settings.actuation_mm &&
            (key_profile->current_mm - key_profile->min_bound_mm) >= settings.rt_press_sensitivity) {
            key_profile->is_pressed = true;
            key_profile->max_bound_mm = key_profile->current_mm;
            Keyboard.press(key);
        }
    } else {
        // Track the deepest point reached while pressed so upward movement can
        // release the key without requiring a return to the actuation point.
        if (key_profile->current_mm > key_profile->max_bound_mm) {
            key_profile->max_bound_mm = key_profile->current_mm;
        }

        if ((key_profile->max_bound_mm - key_profile->current_mm) >= settings.rt_release_sensitivity ||
            key_profile->current_mm < settings.actuation_mm) {
            key_profile->is_pressed = false;
            key_profile->min_bound_mm = key_profile->current_mm;
            Keyboard.release(key);
        }
    }
}

inline void isKeyPressed(float distance_mm, RapidTriggerProfile *key_profile, const char* text) { // rapid trigger under the actuation point
    key_profile->current_mm = distance_mm;

    if (!key_profile->is_pressed) {
        // Track the highest point reached while released. Once the switch has
        // crossed the fixed actuation point, downward motion can re-actuate it.
        if (key_profile->current_mm < key_profile->min_bound_mm) {
            key_profile->min_bound_mm = key_profile->current_mm;
        }

        if (key_profile->current_mm >= settings.actuation_mm &&
            (key_profile->current_mm - key_profile->min_bound_mm) >= settings.rt_press_sensitivity) {
            key_profile->is_pressed = true;
            key_profile->max_bound_mm = key_profile->current_mm;
            Keyboard.print(text);
        }
    } else {
        // Track the deepest point reached while pressed so upward movement can
        // release the key without requiring a return to the actuation point.
        if (key_profile->current_mm > key_profile->max_bound_mm) {
            key_profile->max_bound_mm = key_profile->current_mm;
        }

        // Text macros should fire once per physical press. Unlike regular keys,
        // do not re-arm from rapid-trigger movement while the switch is held.
        if (key_profile->current_mm < settings.actuation_mm) {
            key_profile->is_pressed = false;
            key_profile->min_bound_mm = key_profile->current_mm;
        }
    }
}
