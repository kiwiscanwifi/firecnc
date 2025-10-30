/**
 * @file buzzer.h
 * @brief Header for buzzer control functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares the `beep` function, which is used to generate audible feedback
 * through the onboard buzzer.
 */
#ifndef BUZZER_H
#define BUZZER_H

#include "version.h"
#include <Arduino.h>

/**
 * @brief Emits a series of beeps on the specified pin.
 *
 * This function is blocking and uses `tone()` and `delay()`.
 *
 * @param pin The GPIO pin connected to the buzzer.
 * @param times The number of beeps to emit.
 */
void beep(int pin, int times);

#endif // BUZZER_H
