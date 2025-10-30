/**
 * @file sd_tasks.h
 * @brief Header for SD card related FreeRTOS tasks and functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares functions for logging to the SD card, monitoring its
 * usage, and managing the in-memory ring buffer for logs.
 */
#ifndef SD_TASKS_H
#define SD_TASKS_H

#include "version.h"
#include <RingBuf.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Extern declaration for the global log ring buffer
extern RingBuf<char, 2048> logBuffer;

/**
 * @brief Appends a message to the log file on the SD card and the ring buffer.
 *
 * This function is thread-safe and can be called from any task.
 *
 * @param message The message to log.
 */
void log_to_sd(const String& message);

/**
 * @brief FreeRTOS task for monitoring SD card usage.
 *
 * Periodically checks the SD card's free space and triggers a warning
 * if the usage exceeds a configured threshold.
 *
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void sd_monitor_task(void* pvParameters);

/**
 * @brief Formats the SD card and creates the initial directory structure.
 * 
 * This function should be called carefully as it will erase all data.
 */
void format_sd_card();

#endif // SD_TASKS_H
