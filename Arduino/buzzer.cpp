/**
 * @file buzzer.cpp
 * @brief Implementation of buzzer control functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file contains the implementation of the `beep` function for generating
 * audible feedback using the onboard buzzer.
 */
#include "buzzer.h"
#include "version.h"
#include <Arduino.h>

void beep(int pin, int times) {
    for (int i = 0; i < times; i++) {
        // Generate a tone of 1000 Hz for 100ms
        tone(pin, 1000, 100);
        // Delay for 200ms (100ms tone + 100ms pause)
        delay(200);
    }
}
