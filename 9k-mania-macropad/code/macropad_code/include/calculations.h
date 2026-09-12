#pragma once

#include <Keyboard.h>
#include <vector>
#include <cmath>

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

struct SwitchProfile {
    float total_travel_mm;
    std::vector<float> LUT;
};

extern SettingsProfile settings;

inline std::vector<float> createLUT(int lut_size, float total_travel, float exponential_factor = 2.5f) {
    std::vector<float> LUT(lut_size);  // Allocates lut_size elements

    for (int i = 0; i < lut_size; i++) {
        float raw_pct = static_cast<float>(i) /
                        static_cast<float>(lut_size - 1);

        float linearized_pct =
            1.0f - std::pow(1.0f - raw_pct, exponential_factor);

        LUT[i] = linearized_pct * total_travel; // LUT values are mm
    }

    return LUT;
}

inline void setupCalculations(float actuation_mm, float top_deadband_mm, float bottom_deadband_mm, float rt_press_sensitivity, float rt_release_sensitivity) { // setup calculations
    settings.actuation_mm = actuation_mm;
    settings.top_deadband_mm = top_deadband_mm;
    settings.bottom_deadband_mm = bottom_deadband_mm;
    settings.rt_press_sensitivity = rt_press_sensitivity;
    settings.rt_release_sensitivity = rt_release_sensitivity;
}

inline float normalizeADC(int adc_live, int adc_released, int adc_pressed, bool invert_adc) { // normalize ADC value to 0.0 to 1.0
    if (adc_released == adc_pressed) return 0.0f;

    if (invert_adc) {
        if (adc_live > adc_released) adc_live = adc_released; // clamp
        if (adc_live < adc_pressed) adc_live = adc_pressed;

        return (float)(adc_released - adc_live) / (adc_released - adc_pressed);
    }

    if (adc_live < adc_released) adc_live = adc_released; // clamp
    if (adc_live > adc_pressed) adc_live = adc_pressed;

    return (float)(adc_live - adc_released) / (adc_pressed - adc_released);
}

inline float getDistanceMM(int adc_live, int adc_released, int adc_pressed, SwitchProfile *sw_profile, bool invert_adc) { // convert normalized ADC value to distance
    float normalized_adc = normalizeADC(adc_live, adc_released, adc_pressed, invert_adc);
    
    if (normalized_adc <= 0.0f) return 0.0f; // clamp
    if (normalized_adc >= 1.0f) return sw_profile->total_travel_mm;
    
    float table_position = normalized_adc * (sw_profile->LUT.size() - 1); // get position in lookup table

    int low_index = (int)table_position;
    int high_index = low_index + 1;
    float blend = table_position - (float)low_index;

    // Look up the two surrounding linear percentage values
    float y0 = sw_profile->LUT[low_index];
    float y1 = sw_profile->LUT[high_index];



    // // Perform the standard linear interpolation formula: y = y0 + blend * (y1 - y0)
    // float linearized_pct = y0 + blend * (y1 - y0);

    // // Convert the finalized percentage to physical millimeters
    // float travel_distance = linearized_pct * sw_profile->total_travel_mm;

    // // Apply deadbands to the top and bottom of the switch
    // if (travel_distance < settings.top_deadband_mm) return 0.0f;
    // if (travel_distance > (sw_profile->total_travel_mm - settings.bottom_deadband_mm)) return sw_profile->total_travel_mm;
    
    // return travel_distance;



    //Perform standard linear interpolation formula
    float distance_mm = y0 + blend * (y1 - y0);

    if (distance_mm < settings.top_deadband_mm) return 0.0f;
    if (distance_mm > sw_profile->total_travel_mm - settings.bottom_deadband_mm) {
        return sw_profile->total_travel_mm;
    }

    return distance_mm;
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
