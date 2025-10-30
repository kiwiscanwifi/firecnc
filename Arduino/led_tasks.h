/**
 * @file led_tasks.h
 * @brief Header for LED-related FreeRTOS tasks and utility functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares the main LED task, helper functions for various LED effects,
 * and the FreeRTOS synchronization objects required for cross-task communication.
 */
#ifndef LED_TASKS_H
#define LED_TASKS_H

#include "version.h"
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Global extern declarations for LED arrays
extern CRGB* ledsY;
extern CRGB* ledsYY;
extern CRGB* ledsX;

// Extern declarations for Alexa brightness control variables
extern volatile uint8_t alexa_brightness_y;
extern volatile uint8_t alexa_brightness_yy;
extern volatile uint8_t alexa_brightness_x;

// Extern declaration for the semaphore used to signal effects from Alexa callbacks
extern SemaphoreHandle_t ledEffectSemaphore;

// Global variables for tracking servo position and idle state
extern int servoY_position;
extern int servoYY_position;
extern int servoX_position;
extern TickType_t last_move_time_Y;
extern TickType_t last_move_time_YY;
extern TickType_t last_move_time_X;

// Message structure for limit switch status changes
struct LimitStatusMessage {
    int strip_id;   // 0 for Y, 1 for YY, 2 for X
    bool min_limit;
    bool max_limit;
};

// Queue for communication between servo_tasks and led_tasks
extern QueueHandle_t ledCommandQueue;

/**
 * @brief FreeRTOS task to manage all LED animations and effects.
 *
 * This task is responsible for running the boot-up animation, handling limit switch
 * indicators, axis position displays, idle dimming, and Alexa-triggered effects.
 *
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void led_task(void* pvParameters);

/**
 * @brief Displays a visual error state on all LED strips.
 *
 * Flashes all strips red for 10 seconds, then holds solid red. This is
 * used for critical errors like SD card failure.
 */
void trigger_sd_error_visual();

/**
 * @brief Performs a crossfade animation to solid blue on all strips.
 *
 * This function is non-blocking and used during system shutdown.
 *
 * @param duration_ms The duration of the crossfade animation in milliseconds.
 */
void crossfade_to_blue(int duration_ms);

/**
 * @brief Displays two short blue flashes on the onboard LED.
 */
void two_short_blue_flashes();

/**
 * @brief Displays a green flash on the onboard LED for a specific duration.
 *
 * @param duration_ms The duration of the flash in milliseconds.
 */
void green_flash(int duration_ms);

/**
 * @brief Sets the LED strips for limit switch visualization.
 * 
 * @param led_strip_index Index of the LED strip (0=Y, 1=YY, 2=X).
 * @param min_limit True if min limit is active.
 * @param max_limit True if max limit is active.
 */
void set_limit_visuals(int led_strip_index, bool min_limit, bool max_limit);

#endif // LED_TASKS_H
