/**
 * @file snmp_tasks.cpp
 * @brief Implementation of SNMP agent and trap functions for fireCNC.
 *
 * Project: fireCNC
 * Version: 0.0.1
 * Author: Mark Dyer
 * Location: Blenheim, New Zealand
 * Contact: intelliservenz@gmail.com
 *
 * This module uses the Arduino_SNMP library to implement an SNMP agent and
 * send traps. It listens for SNMP GET requests and provides custom data
 * like system status, and sends traps for events such as startups and errors.
 */
#include "snmp_tasks.h"
#include "version.h"
#include "config.h"
#include "sd_tasks.h"
#include "pins.h"
#include "networking.h"
#include <SNMP_Agent.h>
#include <WiFi.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <esp_adc_cal.h>
#include <esp_chip_info.h>
#include <esp_task_wdt.h>

// UDP object for SNMP communication
WiFiUDP snmp_udp;

// The SNMP Agent instance
SNMPAgent snmp;

// OIDs for custom variables
const char* OID_STATUS = "1.3.6.1.4.1.54021.10.1.1";
const char* OID_VERSION = "1.3.6.1.4.1.54021.10.1.2";
const char* OID_UPTIME = "1.3.6.1.4.1.54021.10.2.1";
const char* OID_TEMPERATURE = "1.3.6.1.4.1.54021.10.2.2";
const char* OID_ADC_VOLTAGE = "1.3.6.1.4.1.54021.10.2.3";
const char* OID_SD_TOTAL = "1.3.6.1.4.1.54021.10.3.1";
const char* OID_SD_USED = "1.3.6.1.4.1.54021.10.3.2";
const char* OID_SD_FREE_PERCENT = "1.3.6.1.4.1.54021.10.3.3";

// Global variables for SNMP data
char system_status[128] = "System is operational.";

// Internal function to get uptime string
void get_uptime_string(char* buffer, size_t size) {
    unsigned long uptime_ms = millis();
    unsigned long seconds = uptime_ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;

    snprintf(buffer, size, "%lu days, %lu hours, %lu minutes, %lu seconds",
             days, hours % 24, minutes % 60, seconds % 60);
}

// Callback for system uptime
int uptimeCallback(SNMP_Value& value, const OID& oid) {
    char uptime_buffer[100];
    get_uptime_string(uptime_buffer, sizeof(uptime_buffer));
    value.setString(uptime_buffer);
    return SNMP_Value::SUCCESS;
}

// Callback for built-in temperature sensor
int temperatureCallback(SNMP_Value& value, const OID& oid) {
    // Note: ESP32 built-in sensor is not highly accurate
    temperature_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temperature_sensor_install(&temp_sensor);
    temperature_sensor_enable();
    float tsens_value;
    temperature_sensor_get_celsius(&tsens_value);
    temperature_sensor_disable();
    value.setFloat(tsens_value);
    return SNMP_Value::SUCCESS;
}

// Callback for ADC voltage
int adcVoltageCallback(SNMP_Value& value, const OID& oid) {
    // Assuming a simple ADC reading on a specific pin (e.g., GPIO34)
    int raw_adc = analogRead(34);
    float voltage = (float)raw_adc / 4095.0 * 3.3; // Convert raw to voltage
    value.setFloat(voltage);
    return SNMP_Value::SUCCESS;
}

// Callback for SD card total space
int sdTotalCallback(SNMP_Value& value, const OID& oid) {
    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        if (SD.cardSize() > 0) {
            value.setUnsigned64(SD.cardSize());
        } else {
            value.setUnsigned64(0);
        }
        xSemaphoreGive(sdMutex);
    } else {
        value.setUnsigned64(0);
    }
    return SNMP_Value::SUCCESS;
}

// Callback for SD card used space
int sdUsedCallback(SNMP_Value& value, const OID& oid) {
    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        if (SD.cardSize() > 0) {
            value.setUnsigned64(SD.usedBytes());
        } else {
            value.setUnsigned64(0);
        }
        xSemaphoreGive(sdMutex);
    } else {
        value.setUnsigned64(0);
    }
    return SNMP_Value::SUCCESS;
}

// Callback for SD card free percentage
int sdFreePercentCallback(SNMP_Value& value, const OID& oid) {
    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {
        if (SD.cardSize() > 0) {
            uint64_t total = SD.cardSize();
            uint64_t used = SD.usedBytes();
            float free_percent = (float)(total - used) * 100.0 / total;
            value.setFloat(free_percent);
        } else {
            value.setFloat(0.0);
        }
        xSemaphoreGive(sdMutex);
    } else {
        value.setFloat(0.0);
    }
    return SNMP_Value::SUCCESS;
}

/**
 * @brief Initializes and starts the SNMP agent.
 */
void snmp_init() {
    snmp.setUDP(&snmp_udp);
    snmp.begin(config.SNMP_COMMUNITY.c_str(), config.SNMP_TRAP_COMMUNITY.c_str());

    // Add custom OID handlers
    snmp.addReadOnlyStringHandler(OID_STATUS, systemStatusCallback);
    snmp.addReadOnlyStringHandler(OID_VERSION, firmwareVersionCallback);
    snmp.addReadOnlyStringHandler(OID_UPTIME, uptimeCallback);
    snmp.addReadOnlyFloatHandler(OID_TEMPERATURE, temperatureCallback);
    snmp.addReadOnlyFloatHandler(OID_ADC_VOLTAGE, adcVoltageCallback);
    snmp.addReadOnlyCounter64Handler(OID_SD_TOTAL, sdTotalCallback);
    snmp.addReadOnlyCounter64Handler(OID_SD_USED, sdUsedCallback);
    snmp.addReadOnlyFloatHandler(OID_SD_FREE_PERCENT, sdFreePercentCallback);

    // Initialize ADC for ADC voltage readings
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Assuming GPIO34

    log_to_sd("SNMP agent initialized.");
}

/**
 * @brief Sends an SNMP trap with a specified message.
 *
 * @param message The message to include in the trap payload.
 */
void snmp_trap_send(const String& message) {
    if (config.SNMP_TRAP_TARGET.length() > 0) {
        IPAddress trap_target_ip;
        if (trap_target_ip.fromString(config.SNMP_TRAP_TARGET)) {
            // Get local IP address based on connection type
            IPAddress local_ip = last_connection_is_ethernet ? ETH.localIP() : WiFi.localIP();
            
            snmp.trap(
                SNMP_Version::SNMP_V2C,
                trap_target_ip,
                config.SNMP_TRAP_COMMUNITY.c_str(),
                "1.3.6.1.4.1.54021.1", // Standard trap OID
                "1.3.6.1.4.1.54021.1.0.1", // Generic trap type (e.g., coldStart)
                message.c_str()
            );
        }
    }
}

/**
 * @brief FreeRTOS task for the SNMP agent.
 *
 * This task runs the main SNMP agent loop, handling incoming requests
 * and managing the agent's state.
 *
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void snmp_agent_task(void* pvParameters) {
    while(1) {
        snmp.loop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
