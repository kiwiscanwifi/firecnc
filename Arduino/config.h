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

// Project details for headers
#define PROJECT_AUTHOR "Mark Dyer"
#define PROJECT_LOCATION "Blenheim, New Zealand"
#define PROJECT_CONTACT "intelliservenz@gmail.com"

/**
 * @struct Config
 * @brief Structure to hold all application configuration settings.
 */
struct Config {

    // SD monitor setting
    struct SD {
        char LOG_FILE_PATH[64];
        int SD_MONITOR_INTERVAL;
        int SD_USAGE_THRESHOLD;
    } SD;
    struct PIN {
        int LEDY_PIN;
        int LEDYY_PIN;
        int LEDX_PIN;
        int RS485_TX_PIN;
        int RS485_RX_PIN;
        int ONBOARD_LED;
    } PIN;

    // LED settings
    struct LEDS {
        int LEDS_Y_COUNT;
        int LEDS_YY_COUNT;
        int LEDS_X_COUNT;

        int FLASH_SPEED;
        int CHASE_SPEED;

        int DEFAULT_BRIGHTNESS_Y;
        int DEFAULT_BRIGHTNESS_YY;
        int DEFAULT_BRIGHTNESS_X;

        int AXIS_POSITION_DISPLAY_LEDS;

        int LED_IDLE_SERVO_DIM;
        int LED_IDLE_SERVO_SECONDS;
    } LEDS;

    // Network settings
    struct NETWORK {
        bool WIFI;
        String WIFI_SSID;
        String WIFI_PASSWORD;
        bool ETHERNET;
        String STATIC_IP;
        String SUBNET;
        String GATEWAY;
        String DNS_SERVER;
        char NTP_SERVER[64];
    } NETWORK;

    // SNMP settings
    struct SNMP {
        bool ENABLED;
        char SNMP_COMMUNITY[64];
        int SNMP_PORT;
        char SNMP_PROTOCOL[3];
        char SNMP_TRAP_COMMUNITY[64];
        char SNMP_TRAP_TARGET[64];
        int TRAP_PORT;
    } SNMP;

    struct Mqtt {
        bool ENABLED;
    } Mqtt;

    struct SYSTEM {
        int WATCHDOG_TIMEOUT;
    } SYSTEM;

    struct TABLE {
        int RAIL_Y_LENGTH;
        int RAIL_X_LENGTH;
        int RAIL_Z_LENGTH;
    } TABLE;

    // Servo settings
    struct SERVOS {
        int SERVOY_SLAVE_ID;
        int SERVOYY_SLAVE_ID;
        int SERVOX_SLAVE_ID;
    } SERVOS;
















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
