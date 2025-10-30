/**
 * @file servo_tasks.h
 * @brief Header for RS485 servo communication and related functions.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares functions for interacting with the LC10e servos via Modbus
 * over RS485, including reading position and limit switch status. It also
 * defines the message structure and queue for inter-task communication.
 */
#ifndef SERVO_TASKS_H
#define SERVO_TASKS_H

#include "version.h"
#include <ModbusMaster.h>

/**
 * @brief FreeRTOS task to manage RS485 communication with servo drivers.
 * 
 * This task is responsible for periodically polling the servo drivers for
 * their status, position, and limit switch states.
 * 
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void servo_task(void* pvParameters);

/**
 * @brief Reads the limit switch status from a servo driver over RS485.
 * 
 * This function is internal to the servo_task.
 *
 * @param node The ModbusMaster instance for the servo.
 * @return A 16-bit word containing the status.
 */
uint16_t read_limit_switches(ModbusMaster& node);

/**
 * @brief Reads the current position from a servo driver over RS485.
 *
 * @param node The ModbusMaster instance for the servo.
 * @return The 32-bit position value.
 */
int32_t read_current_position(ModbusMaster& node);

#endif // SERVO_TASKS_H
