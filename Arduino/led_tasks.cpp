/**
 * @file led_tasks.cpp
 * @brief Implementation of LED-related FreeRTOS tasks and utility functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file contains the logic for all LED animations and effects, managing
 * the FastLED library, and handling signals from other tasks for visual feedback.
 */
#include "led_tasks.h"
#include "version.h"
#include "config.h"
#include "pins.h"
#include "buzzer.h"
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h> // For memcpy
#include <TCA9554.h>

// Global extern declarations for LED arrays
extern CRGB* ledsY;
extern CRGB* ledsYY;
extern CRGB* ledsX;

// Extern declarations for Alexa brightness control variables
extern volatile uint8_t alexa_brightness_y;
extern volatile uint8_t alexa_brightness_yy;
extern volatile uint8_t alexa_brightness_x;
extern TCA9554 tca9554;

// FreeRTOS synchronization objects
SemaphoreHandle_t ledEffectSemaphore;
QueueHandle_t ledCommandQueue;

// Global variables for tracking servo position and idle state
int servoY_position = 0;
int servoYY_position = 0;
int servoX_position = 0;
TickType_t last_move_time_Y = 0;
TickType_t last_move_time_YY = 0;
TickType_t last_move_time_X = 0;

// Internal state for the LED effects
enum LedEffect {
    NO_EFFECT,
    BOOT_ANIMATION,
    SD_ERROR,
    CHASE_PURPLE,
    CROSSFADE_BLUE
};
volatile LedEffect currentEffect = BOOT_ANIMATION;
volatile bool sd_error_active = false;
volatile bool chasing_purple_active = false;

// Backup arrays for storing LED state before position display
CRGB ledsY_backup[config.LEDS_Y_COUNT];
CRGB ledsYY_backup[config.LEDS_YY_COUNT];
CRGB ledsX_backup[config.LEDS_X_COUNT];

// Variables to track the last drawn position
int last_pos_Y = -1;
int last_pos_YY = -1;
int last_pos_X = -1;

// Limit switch status, updated by the queue
bool min_limit_Y = false;
bool max_limit_Y = false;
bool min_limit_YY = false;
bool max_limit_YY = false;
bool min_limit_X = false;
bool max_limit_X = false;


// Helper functions for animations
void knight_rider_effect(CRGB* leds, int num_leds, CRGB color, int speed);
void chasing_effect(CRGB* leds, int num_leds, CRGB color, int speed);
void dim_leds_on_idle(CRGB* leds, int num_leds, int idle_percent);
void flash_onboard_led(int pin, CRGB color, int duration_ms, int speed_ms);
void update_position_display_and_preserve(CRGB* leds, CRGB* backup_leds, int num_leds, int position, int rail_length, int led_count_around_center, int& last_pos);
void flash_red_limits(CRGB* leds, int num_leds, bool min_limit, bool max_limit);


/**
 * @brief Sets the LED strips for limit switch visualization.
 * 
 * @param led_strip_index Index of the LED strip (0=Y, 1=YY, 2=X).
 * @param min_limit True if min limit is active.
 * @param max_limit True if max limit is active.
 */
void set_limit_visuals(int led_strip_index, bool min_limit, bool max_limit) {
    // This function is not directly used in the main loop but can be
    // called by other tasks to signal state changes. The actual visual
    // update is handled in the `led_task`.
}

/**
 * @brief Flashes red on limit switches and sets remaining LEDs to orange.
 * 
 * @param leds The FastLED array for the strip.
 * @param num_leds The total number of LEDs in the strip.
 * @param min_limit True if the min limit switch is active.
 * @param max_limit True if the max limit switch is active.
 */
void flash_red_limits(CRGB* leds, int num_leds, bool min_limit, bool max_limit) {
    static bool flash_state = false;
    static unsigned long last_flash = 0;
    const int flash_delay = 500; // milliseconds

    if (millis() - last_flash > flash_delay) {
        flash_state = !flash_state;
        last_flash = millis();
    }
    
    // Set all remaining LEDs to solid orange
    fill_solid(leds, num_leds, CRGB::Orange);
    
    if (min_limit && flash_state) {
        fill_solid(leds, 20, CRGB::Red); // First 20 LEDs flash red
    } else if (min_limit && !flash_state) {
        fill_solid(leds, 20, CRGB::Orange);
    }
    
    if (max_limit && flash_state) {
        fill_solid(leds + (num_leds - 20), 20, CRGB::Red); // Last 20 LEDs flash red
    } else if (max_limit && !flash_state) {
        fill_solid(leds + (num_leds - 20), 20, CRGB::Orange);
    }
}


/**
 * @brief FreeRTOS task to manage all LED animations and effects.
 */
void led_task(void* pvParameters) {
    ledEffectSemaphore = xSemaphoreCreateBinary();
    ledCommandQueue = xQueueCreate(10, sizeof(LimitStatusMessage)); // Create the queue

    // Boot-up animation: Knight Rider on all strips
    log_to_sd("Starting LED boot-up animation.");
    long start_time = millis();
    while (millis() - start_time < 10000) {
        knight_rider_effect(ledsY, config.LEDS_Y_COUNT, CRGB::Blue, 50);
        knight_rider_effect(ledsYY, config.LEDS_YY_COUNT, CRGB::Blue, 50);
        knight_rider_effect(ledsX, config.LEDS_X_COUNT, CRGB::Blue, 50);
        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(config.FLASH_SPEED));
    }
    log_to_sd("LED boot-up animation complete.");

    // After boot-up, set LEDY and LEDYY to solid white
    fill_solid(ledsY, config.LEDS_Y_COUNT, CRGB::White);
    fill_solid(ledsYY, config.LEDS_YY_COUNT, CRGB::White);
    FastLED.show();

    // Make sure backup arrays are initialized
    memcpy(ledsY_backup, ledsY, sizeof(CRGB) * config.LEDS_Y_COUNT);
    memcpy(ledsYY_backup, ledsYY, sizeof(CRGB) * config.LEDS_YY_COUNT);
    memcpy(ledsX_backup, ledsX, sizeof(CRGB) * config.LEDS_X_COUNT);


    while (1) {
        // Process incoming messages from other tasks
        LimitStatusMessage msg;
        if (xQueueReceive(ledCommandQueue, &msg, 0) == pdTRUE) {
            switch (msg.strip_id) {
                case 0: // LEDY
                    min_limit_Y = msg.min_limit;
                    max_limit_Y = msg.max_limit;
                    break;
                case 1: // LEDYY
                    min_limit_YY = msg.min_limit;
                    max_limit_YY = msg.max_limit;
                    break;
                case 2: // LEDX
                    min_limit_X = msg.min_limit;
                    max_limit_X = msg.max_limit;
                    break;
            }
        }

        // Apply limit switch visual indicators using the stored state
        flash_red_limits(ledsY, config.LEDS_Y_COUNT, min_limit_Y, max_limit_Y);
        flash_red_limits(ledsYY, config.LEDS_YY_COUNT, min_limit_YY, max_limit_YY);
        flash_red_limits(ledsX, config.LEDS_X_COUNT, min_limit_X, max_limit_X);
        
        // Update servo position display
        update_position_display_and_preserve(ledsY, ledsY_backup, config.LEDS_Y_COUNT, servoY_position, config.RAIL_Y_LENGTH_MM, config.AXIS_POSITION_DISPLAY_LEDS, last_pos_Y);
        update_position_display_and_preserve(ledsYY, ledsYY_backup, config.LEDS_YY_COUNT, servoYY_position, config.RAIL_Y_LENGTH_MM, config.AXIS_POSITION_DISPLAY_LEDS, last_pos_YY);
        update_position_display_and_preserve(ledsX, ledsX_backup, config.LEDS_X_COUNT, servoX_position, config.RAIL_X_LENGTH_MM, config.AXIS_POSITION_DISPLAY_LEDS, last_pos_X);


        // Idle dimming
        if (xTaskGetTickCount() - last_move_time_Y > pdMS_TO_TICKS(config.IDLE_TIMEOUT_SECONDS * 1000)) {
            dim_leds_on_idle(ledsY, config.LEDS_Y_COUNT, config.IDLE_DIM_PERCENT);
        }
        if (xTaskGetTickCount() - last_move_time_YY > pdMS_TO_TICKS(config.IDLE_TIMEOUT_SECONDS * 1000)) {
            dim_leds_on_idle(ledsYY, config.LEDS_YY_COUNT, config.IDLE_DIM_PERCENT);
        }
        if (xTaskGetTickCount() - last_move_time_X > pdMS_TO_TICKS(config.IDLE_TIMEOUT_SECONDS * 1000)) {
            dim_leds_on_idle(ledsX, config.LEDS_X_COUNT, config.IDLE_DIM_PERCENT);
        }

        // Apply Alexa brightness
        FastLED.setBrightness(alexa_brightness_y);
        FastLED.show(ledsY, config.LEDS_Y_COUNT);
        FastLED.setBrightness(alexa_brightness_yy);
        FastLED.show(ledsYY, config.LEDS_YY_COUNT);
        FastLED.setBrightness(alexa_brightness_x);
        FastLED.show(ledsX, config.LEDS_X_COUNT);

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(100)); // Standard delay for the task loop
    }
}

/**
 * @brief Updates the LED strip to display the current servo position, preserving the previous state.
 * @param leds The FastLED array for the strip.
 * @param backup_leds The backup array for the strip.
 * @param num_leds The total number of LEDs in the strip.
 * @param position The current servo position in millimeters.
 * @param rail_length The total rail length in millimeters.
 * @param led_count_around_center The number of green LEDs on either side of the center.
 * @param last_pos The last displayed position.
 */
void update_position_display_and_preserve(CRGB* leds, CRGB* backup_leds, int num_leds, int position, int rail_length, int led_count_around_center, int& last_pos) {
    if (rail_length <= 0) return;

    // Calculate the current LED position
    int led_pos = (position * num_leds) / rail_length;
    
    // If position has not changed, do nothing
    if (led_pos == last_pos) return;

    // Restore previous state if a previous position existed
    if (last_pos != -1) {
        int start_restore = max(0, last_pos - led_count_around_center);
        int end_restore = min(num_leds - 1, last_pos + led_count_around_center);
        for (int i = start_restore; i <= end_restore; i++) {
            leds[i] = backup_leds[i];
        }
    }

    // Save the state of the new LEDs before changing their color
    int start_save = max(0, led_pos - led_count_around_center);
    int end_save = min(num_leds - 1, led_pos + led_count_around_center);
    for (int i = start_save; i <= end_save; i++) {
        backup_leds[i] = leds[i];
    }
    
    // Turn on the new green center
    for (int i = start_save; i <= end_save; i++) {
        leds[i] = CRGB::Green;
    }

    last_pos = led_pos;
}


/**
 * @brief Displays a visual error state on all LED strips.
 */
void trigger_sd_error_visual() {
    log_to_sd("Triggering SD error visual.");
    beep(BUZZER_PIN, 3);
    long start_time = millis();
    while (millis() - start_time < 10000) {
        fill_solid(ledsY, config.LEDS_Y_COUNT, CRGB::Red);
        fill_solid(ledsYY, config.LEDS_YY_COUNT, CRGB::Red);
        fill_solid(ledsX, config.LEDS_X_COUNT, CRGB::Red);
        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(config.FLASH_SPEED));
        fill_solid(ledsY, config.LEDS_Y_COUNT, CRGB::Black);
        fill_solid(ledsYY, config.LEDS_YY_COUNT, CRGB::Black);
        fill_solid(ledsX, config.LEDS_X_COUNT, CRGB::Black);
        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(config.FLASH_SPEED));
    }
    fill_solid(ledsY, config.LEDS_Y_COUNT, CRGB::Red);
    fill_solid(ledsYY, config.LEDS_YY_COUNT, CRGB::Red);
    fill_solid(ledsX, config.LEDS_X_COUNT, CRGB::Red);
    FastLED.show();
}

/**
 * @brief Performs a crossfade animation to solid blue on all strips.
 */
void crossfade_to_blue(int duration_ms) {
    const int num_steps = 100;
    const int step_delay = duration_ms / num_steps;
    
    CRGB final_color = CRGB::Blue;
    
    CRGB initial_ledsY[config.LEDS_Y_COUNT];
    CRGB initial_ledsYY[config.LEDS_YY_COUNT];
    CRGB initial_ledsX[config.LEDS_X_COUNT];
    
    for (int i = 0; i < config.LEDS_Y_COUNT; i++) initial_ledsY[i] = ledsY[i];
    for (int i = 0; i < config.LEDS_YY_COUNT; i++) initial_ledsYY[i] = ledsYY[i];
    for (int i = 0; i < config.LEDS_X_COUNT; i++) initial_ledsX[i] = ledsX[i];
    
    for (int i = 0; i <= num_steps; i++) {
        float blend_factor = (float)i / num_steps;

        for (int j = 0; j < config.LEDS_Y_COUNT; j++) {
            ledsY[j] = blend(initial_ledsY[j], final_color, blend_factor * 255);
        }
        for (int j = 0; j < config.LEDS_YY_COUNT; j++) {
            ledsYY[j] = blend(initial_ledsYY[j], final_color, blend_factor * 255);
        }
        for (int j = 0; j < config.LEDS_X_COUNT; j++) {
            ledsX[j] = blend(initial_ledsX[j], final_color, blend_factor * 255);
        }

        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(step_delay));
    }
}

/**
 * @brief Displays two short blue flashes on the onboard LED.
 */
void two_short_blue_flashes() {
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(100);
    digitalWrite(ONBOARD_LED, LOW);
    delay(100);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(100);
    digitalWrite(ONBOARD_LED, LOW);
}

/**
 * @brief Displays a green flash on the onboard LED for a specific duration.
 */
void green_flash(int duration_ms) {
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(duration_ms);
    digitalWrite(ONBOARD_LED, LOW);
}

/**
 * @brief Runs a Knight Rider effect on an LED strip.
 */
void knight_rider_effect(CRGB* leds, int num_leds, CRGB color, int speed) {
    static int head = 0;
    static bool forward = true;
    
    // Clear the strip
    fill_solid(leds, num_leds, CRGB::Black);

    // Draw the "eye"
    leds[head] = color;
    
    // Move the eye
    if (forward) {
        head++;
        if (head >= num_leds - 1) {
            forward = false;
        }
    } else {
        head--;
        if (head <= 0) {
            forward = true;
        }
    }
    
    // Delay for speed
    vTaskDelay(pdMS_TO_TICKS(speed));
}

/**
 * @brief Runs a chasing effect on an LED strip.
 */
void chasing_effect(CRGB* leds, int num_leds, CRGB color, int speed) {
    static int pos = 0;
    fill_solid(leds, num_leds, CRGB::Black);
    leds[pos] = color;
    pos = (pos + 1) % num_leds;
    vTaskDelay(pdMS_TO_TICKS(speed));
}


/**
 * @brief Dims the LEDs on an idle axis.
 */
void dim_leds_on_idle(CRGB* leds, int num_leds, int idle_percent) {
    for (int i = 0; i < num_leds; i++) {
        // Only dim white LEDs
        if (leds[i] == CRGB::White) {
            leds[i].nscale8(idle_percent);
        }
    }
}
