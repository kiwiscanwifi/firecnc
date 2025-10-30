/**
 * @file sd_tasks.cpp
 * @brief Implementation of SD card related FreeRTOS tasks and functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This module handles logging to the SD card, monitoring SD card space,
 * and managing an in-memory ring buffer for recent logs.
 */
#include "sd_tasks.h"
#include "version.h"
#include "config.h"
#include "pins.h"
#include "led_tasks.h"
#include "snmp_tasks.h"
#include <SD.h>
#include <FS.h>
#include <time.h>

#include "freertos/ringbuf.h"
#include <string.h> // For memcpy

// The handle for the ring buffer
RingbufHandle_t logBufferHandle;

// Mutex to protect SD card access
static SemaphoreHandle_t sdMutex;

// Name of the log file
const char* LOG_FILE_PATH = "/system.log";

/**
 * @brief Formats the SD card and creates the initial directory structure.
 * 
 * This function includes SNMP traps and an audible alert for formatting.
 * It should be used with caution as it erases all data on the SD card.
 */
void format_sd_card() {
    snmp_trap_send("SD Card Format Initiated");
    log_to_sd("SD Card formatting initiated.");

    // Long buzzer beep to indicate formatting is in progress
    tone(BUZZER_PIN, 1000, 5000); 

    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        if (SD.format()) {
            log_to_sd("SD card formatted successfully.");
            snmp_trap_send("SD Card Format Successful");
        } else {
            log_to_sd("SD card format failed.");
            snmp_trap_send("SD Card Format Failed");
        }
        xSemaphoreGive(sdMutex);
    }
    
    // Stop the buzzer tone after formatting is complete
    noTone(BUZZER_PIN);
}

/**
 * @brief Appends a message to the log file on the SD card and the ring buffer.
 */
void log_to_sd(const String& message) {
    if (sdMutex == NULL) {
        sdMutex = xSemaphoreCreateMutex();
    }

    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        // Get current timestamp
        struct tm timeinfo;
        time_t now;
        time(&now);
        localtime_r(&now, &timeinfo);
        char timestamp_str[20];
        strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

        // Append to file
        File logFile = SD.open(LOG_FILE_PATH, FILE_APPEND);
        if (logFile) {
            logFile.printf("[%s] %s\n", timestamp_str, message.c_str());
            logFile.close();
        } else {
            // SNMP Trap for SD write failure
            snmp_trap_send("SD Card Write Failed");
        }
        xSemaphoreGive(sdMutex);
    }
}

/**
 * @brief FreeRTOS task for monitoring SD card usage.
 */
void sd_monitor_task(void* pvParameters) {
    // Initial check on startup
    if (SD.cardSize() > 0) {
        uint64_t totalBytes = SD.cardSize() / (1024 * 1024);
        uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
        int usagePercent = (usedBytes * 100) / totalBytes;
        log_to_sd("SD Card: Total " + String(totalBytes) + " MB, Used " + String(usedBytes) + " MB (" + String(usagePercent) + "%)");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(config.SD_MONITOR_INTERVAL * 1000));

        if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
            if (SD.cardSize() > 0) {
                uint64_t totalBytes = SD.cardSize() / (1024 * 1024);
                uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
                int usagePercent = (usedBytes * 100) / totalBytes;

                if (usagePercent > config.SD_USAGE_THRESHOLD) {
                    log_to_sd("WARNING: SD card storage is over " + String(config.SD_USAGE_THRESHOLD) + "% full. Used: " + String(usagePercent) + "%.");
                    // Blink onboard LED red and fast for 20 seconds
                    flash_onboard_led(ONBOARD_LED, CRGB::Red, 20000, 100);
                }
            } else {
                log_to_sd("SD card not available during monitor check.");
                snmp_trap_send("SD Card Monitor Failed");
            }
            xSemaphoreGive(sdMutex);
        }
    }
}

// Function to write initial website files if they don't exist
void setup_web_files() {
    if (!SD.exists("/www")) {
        log_to_sd("Creating /www directory on SD card.");
        SD.mkdir("/www");
    }
}
