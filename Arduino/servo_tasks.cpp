/**
 * @file servo_tasks.cpp
 * @brief Implementation of RS485 servo communication tasks.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This module handles the Modbus communication with the LC10e servo drivers
 * over RS485. It polls for limit switch status and position data,
 * and communicates these to the `led_tasks` using a FreeRTOS queue.
 */
#include "servo_tasks.h"
#include "version.h"
#include "config.h"
#include "pins.h"
#include "led_tasks.h"
#include "snmp_tasks.h"
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Global extern declarations
extern ModbusMaster nodeY;
extern ModbusMaster nodeYY;
extern ModbusMaster nodeX;
extern QueueHandle_t ledCommandQueue;
extern int servoY_position;
extern int servoYY_position;
extern int servoX_position;
extern TickType_t last_move_time_Y;
extern TickType_t last_move_time_YY;
extern TickType_t last_move_time_X;

// HardwareSerial for RS485 communication
HardwareSerial RS485Serial(2); // Using UART2

// Function prototypes for internal use
uint16_t read_limit_switches(ModbusMaster& node);
int32_t read_current_position(ModbusMaster& node);
void check_and_update_position(int servo_position, int& last_position, TickType_t& last_move_time);

/**
 * @brief Reads the limit switch status from a servo driver over RS485.
 * 
 * This implementation assumes register 10 holds the limit switch status.
 *
 * @param node The ModbusMaster instance for the servo.
 * @return A 16-bit word containing the status.
 */
uint16_t read_limit_switches(ModbusMaster& node) {
    uint8_t result = node.readHoldingRegisters(10, 1);
    if (result == node.ku8MBIISuccess) {
        return node.getResponseBuffer(0);
    }
    return 0; // Return 0 on failure
}

/**
 * @brief Reads the current position from a servo driver over RS485.
 *
 * @param node The ModbusMaster instance for the servo.
 * @return The 32-bit position value.
 */
int32_t read_current_position(ModbusMaster& node) {
    uint8_t result = node.readHoldingRegisters(20, 2); // Assuming 32-bit position starts at register 20
    if (result == node.ku8MBIISuccess) {
        uint32_t high_word = node.getResponseBuffer(0);
        uint32_t low_word = node.getResponseBuffer(1);
        return (high_word << 16) | low_word;
    }
    return 0;
}

/**
 * @brief Checks if the position has changed and updates the last move time.
 *
 * @param current_position The current position of the servo.
 * @param last_position Reference to the variable storing the last known position.
 * @param last_move_time Reference to the variable storing the time of the last move.
 */
void check_and_update_position(int current_position, int& last_position, TickType_t& last_move_time) {
    if (current_position != last_position) {
        last_position = current_position;
        last_move_time = xTaskGetTickCount();
    }
}

/**
 * @brief FreeRTOS task to manage RS485 communication.
 */
void servo_task(void* pvParameters) {
    RS485Serial.begin(19200, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
    
    // Set up Modbus nodes
    nodeY.begin(config.SERVOY_SLAVE_ID, RS485Serial);
    nodeYY.begin(config.SERVOYY_SLAVE_ID, RS485Serial);
    nodeX.begin(config.SERVOX_SLAVE_ID, RS485Serial);
    
    // Set RTS pin for direction control
    nodeY.setSlaveControlPin(RS485_RTS_PIN);
    nodeYY.setSlaveControlPin(RS485_RTS_PIN);
    nodeX.setSlaveControlPin(RS485_RTS_PIN);
    
    uint16_t last_status_y = 0;
    uint16_t last_status_yy = 0;
    uint16_t last_status_x = 0;

    while(1) {
        // Poll SERVOY for limit switch status
        uint16_t status_y = read_limit_switches(nodeY);
        if (status_y != last_status_y) {
            LimitStatusMessage msg = {0, (bool)(status_y & 0x01), (bool)(status_y & 0x02)};
            xQueueSend(ledCommandQueue, &msg, 0);
            last_status_y = status_y;
        }

        // Poll SERVOYY for limit switch status
        uint16_t status_yy = read_limit_switches(nodeYY);
        if (status_yy != last_status_yy) {
            LimitStatusMessage msg = {1, (bool)(status_yy & 0x01), (bool)(status_yy & 0x02)};
            xQueueSend(ledCommandQueue, &msg, 0);
            last_status_yy = status_yy;
        }

        // Poll SERVOX for limit switch status
        uint16_t status_x = read_limit_switches(nodeX);
        if (status_x != last_status_x) {
            LimitStatusMessage msg = {2, (bool)(status_x & 0x01), (bool)(status_x & 0x02)};
            xQueueSend(ledCommandQueue, &msg, 0);
            last_status_x = status_x;
        }

        // Poll SERVOY for current position
        int32_t current_pos_y = read_current_position(nodeY);
        check_and_update_position(current_pos_y, servoY_position, last_move_time_Y);

        // Poll SERVOYY for current position
        int32_t current_pos_yy = read_current_position(nodeYY);
        check_and_update_position(current_pos_yy, servoYY_position, last_move_time_YY);

        // Poll SERVOX for current position
        int32_t current_pos_x = read_current_position(nodeX);
        check_and_update_position(current_pos_x, servoX_position, last_move_time_X);

        vTaskDelay(pdMS_TO_TICKS(100)); // Poll more frequently for position tracking
    }
}
