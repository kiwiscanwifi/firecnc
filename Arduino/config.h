/**
 * @file config.h
 * @brief Header for managing project configuration via a JSON file.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares the `Config` struct and functions for loading and saving
 * the configuration from and to a JSON file on the SD card.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include "version.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @struct Config
 * @brief Structure to hold all application configuration settings.
 */
struct Config {
    // Network settings
    String WIFI_SSID;
    String WIFI_PASSWORD;
    String STATIC_IP;
    String SUBNET;
    String GATEWAY;
    String DNS_SERVER;
    String NTP_SERVER;

    // LED settings
    int LEDS_Y_COUNT;
    int LEDS_YY_COUNT;
    int LEDS_X_COUNT;
    int DEFAULT_BRIGHTNESS_Y;
    int DEFAULT_BRIGHTNESS_YY;
    int DEFAULT_BRIGHTNESS_X;
    int AXIS_POSITION_DISPLAY_LEDS;
    int CHASE_SPEED;
    int FLASH_SPEED;
    int IDLE_DIM_PERCENT;
    int IDLE_TIMEOUT_SECONDS;

    // Servo settings
    int SERVOY_SLAVE_ID;
    int SERVOYY_SLAVE_ID;
    int SERVOX_SLAVE_ID;
    int RAIL_Y_LENGTH_MM;
    int RAIL_X_LENGTH_MM;

    // SNMP settings
    String SNMP_COMMUNITY;
    String SNMP_TRAP_COMMUNITY;
    String SNMP_TRAP_TARGET;
    int SNMP_PORT;
    String SNMP_PROTOCOL;

    // SSH settings
    String SSH_USERNAME;
    String SSH_PASSWORD;

    // Watchdog setting
    int WATCHDOG_TIMEOUT;

    // SD monitor setting
    int SD_MONITOR_INTERVAL;
    int SD_USAGE_THRESHOLD;
};

extern Config config;

/**
 * @brief Loads the configuration from a JSON file on the SD card.
 * @return True if the configuration was loaded successfully, false otherwise.
 */
bool load_config_from_sd();

/**
 * @brief Saves the current configuration to a JSON file on the SD card.
 * @return True if the configuration was saved successfully, false otherwise.
 */
bool save_config_to_sd();

#endif // CONFIG_H
