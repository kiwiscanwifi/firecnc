/**
 * @file config.cpp
 * @brief Implements functions for managing project configuration via a JSON file.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file contains the implementation of the configuration loading and saving
 * functions, using the ArduinoJson library for parsing and serialization.
 */
#include "config.h"
#include "sd_tasks.h"
#include <SD.h>

Config config;

/**
 * @brief Loads the configuration from a JSON file on the SD card.
 * @return True if the configuration was loaded successfully, false otherwise.
 */
bool load_config_from_sd() {
    File configFile = SD.open("/config.json");
    if (!configFile) {
        log_to_sd("Failed to open config file for reading");
        return false;
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        log_to_sd("Failed to parse config file: " + String(error.c_str()));
        configFile.close();
        return false;
    }
    configFile.close();

    config.WIFI_SSID = doc["NETWORK"]["WIFI_SSID"].as<String>();
    config.WIFI_PASSWORD = doc["NETWORK"]["WIFI_PASSWORD"].as<String>();
    config.STATIC_IP = doc["NETWORK"]["STATIC_IP"].as<String>();
    config.SUBNET = doc["NETWORK"]["SUBNET"].as<String>();
    config.GATEWAY = doc["NETWORK"]["GATEWAY"].as<String>();
    config.DNS_SERVER = doc["NETWORK"]["DNS_SERVER"].as<String>();
    config.NTP_SERVER = doc["NETWORK"]["NTP_SERVER"].as<String>();

    config.LEDS_Y_COUNT = doc["LEDS"]["LEDS_Y_COUNT"].as<int>();
    config.LEDS_YY_COUNT = doc["LEDS"]["LEDS_YY_COUNT"].as<int>();
    config.LEDS_X_COUNT = doc["LEDS"]["LEDS_X_COUNT"].as<int>();
    config.DEFAULT_BRIGHTNESS_Y = doc["LEDS"]["DEFAULT_BRIGHTNESS_Y"].as<int>();
    config.DEFAULT_BRIGHTNESS_YY = doc["LEDS"]["DEFAULT_BRIGHTNESS_YY"].as<int>();
    config.DEFAULT_BRIGHTNESS_X = doc["LEDS"]["DEFAULT_BRIGHTNESS_X"].as<int>();
    config.AXIS_POSITION_DISPLAY_LEDS = doc["LEDS"]["AXIS_POSITION_DISPLAY_LEDS"].as<int>();
    config.CHASE_SPEED = doc["LEDS"]["CHASE_SPEED"].as<int>();
    config.FLASH_SPEED = doc["LEDS"]["FLASH_SPEED"].as<int>();
    config.IDLE_DIM_PERCENT = doc["LEDS"]["IDLE_DIM_PERCENT"].as<int>();
    config.IDLE_TIMEOUT_SECONDS = doc["LEDS"]["IDLE_TIMEOUT_SECONDS"].as<int>();

    config.SERVOY_SLAVE_ID = doc["SERVOS"]["SERVOY_SLAVE_ID"].as<int>();
    config.SERVOYY_SLAVE_ID = doc["SERVOS"]["SERVOYY_SLAVE_ID"].as<int>();
    config.SERVOX_SLAVE_ID = doc["SERVOS"]["SERVOX_SLAVE_ID"].as<int>();

    config.RAIL_Y_LENGTH_MM = doc["SERVOS"]["RAIL_Y_LENGTH_MM"].as<int>();
    config.RAIL_X_LENGTH_MM = doc["SERVOS"]["RAIL_X_LENGTH_MM"].as<int>();

    config.SNMP_COMMUNITY = doc["SNMP"]["SNMP_COMMUNITY"].as<String>();
    config.SNMP_TRAP_COMMUNITY = doc["SNMP"]["SNMP_TRAP_COMMUNITY"].as<String>();
    config.SNMP_TRAP_TARGET = doc["SNMP"]["SNMP_TRAP_TARGET"].as<String>();
    config.SNMP_PORT = doc["SNMP"]["SNMP_PORT"].as<int>();
    config.SNMP_PROTOCOL = doc["SNMP"]["SNMP_PROTOCOL"].as<String>();

    config.SSH_USERNAME = doc["SSH"]["SSH_USERNAME"].as<String>();
    config.SSH_PASSWORD = doc["SSH"]["SSH_PASSWORD"].as<String>();

    config.WATCHDOG_TIMEOUT = doc["WATCHDOG"]["WATCHDOG_TIMEOUT"].as<int>();

    config.SD_MONITOR_INTERVAL = doc["SD"]["SD_MONITOR_INTERVAL"].as<int>();
    config.SD_USAGE_THRESHOLD = doc["SD"]["SD_USAGE_THRESHOLD"].as<int>();
    
    return true;
}

/**
 * @brief Saves the current configuration to a JSON file on the SD card.
 * @return True if the configuration was saved successfully, false otherwise.
 */
bool save_config_to_sd() {
    File configFile = SD.open("/config.json", FILE_WRITE);
    if (!configFile) {
        log_to_sd("Failed to open config file for writing");
        return false;
    }

    StaticJsonDocument<2048> doc;
    doc["NETWORK"]["WIFI_SSID"] = config.WIFI_SSID;
    doc["NETWORK"]["WIFI_PASSWORD"] = config.WIFI_PASSWORD;
    doc["NETWORK"]["STATIC_IP"] = config.STATIC_IP;
    doc["NETWORK"]["SUBNET"] = config.SUBNET;
    doc["NETWORK"]["GATEWAY"] = config.GATEWAY;
    doc["NETWORK"]["DNS_SERVER"] = config.DNS_SERVER;
    doc["NETWORK"]["NTP_SERVER"] = config.NTP_SERVER;

    doc["LEDS"]["LEDS_Y_COUNT"] = config.LEDS_Y_COUNT;
    doc["LEDS"]["LEDS_YY_COUNT"] = config.LEDS_YY_COUNT;
    doc["LEDS"]["LEDS_X_COUNT"] = config.LEDS_X_COUNT;
    doc["LEDS"]["DEFAULT_BRIGHTNESS_Y"] = config.DEFAULT_BRIGHTNESS_Y;
    doc["LEDS"]["DEFAULT_BRIGHTNESS_YY"] = config.DEFAULT_BRIGHTNESS_YY;
    doc["LEDS"]["DEFAULT_BRIGHTNESS_X"] = config.DEFAULT_BRIGHTNESS_X;
    doc["LEDS"]["AXIS_POSITION_DISPLAY_LEDS"] = config.AXIS_POSITION_DISPLAY_LEDS;
    doc["LEDS"]["CHASE_SPEED"] = config.CHASE_SPEED;
    doc["LEDS"]["FLASH_SPEED"] = config.FLASH_SPEED;
    doc["LEDS"]["IDLE_DIM_PERCENT"] = config.IDLE_DIM_PERCENT;
    doc["LEDS"]["IDLE_TIMEOUT_SECONDS"] = config.IDLE_TIMEOUT_SECONDS;

    doc["SERVOS"]["SERVOY_SLAVE_ID"] = config.SERVOY_SLAVE_ID;
    doc["SERVOS"]["SERVOYY_SLAVE_ID"] = config.SERVOYY_SLAVE_ID;
    doc["SERVOS"]["SERVOX_SLAVE_ID"] = config.SERVOX_SLAVE_ID;

    doc["SERVOS"]["RAIL_Y_LENGTH_MM"] = config.RAIL_Y_LENGTH_MM;
    doc["SERVOS"]["RAIL_X_LENGTH_MM"] = config.RAIL_X_LENGTH_MM;


    doc["SNMP"]["SNMP_COMMUNITY"] = config.SNMP_COMMUNITY;
    doc["SNMP"]["SNMP_TRAP_COMMUNITY"] = config.SNMP_TRAP_COMMUNITY;
    doc["SNMP"]["SNMP_TRAP_TARGET"] = config.SNMP_TRAP_TARGET;
    doc["SNMP"]["SNMP_PORT"] = config.SNMP_PORT;
    doc["SNMP"]["SNMP_PROTOCOL"] = config.SNMP_PROTOCOL;

    doc["SSH"]["SSH_USERNAME"] = config.SSH_USERNAME;
    doc["SSH"]["SSH_PASSWORD"] = config.SSH_PASSWORD;

    doc["WATCHDOG"]["WATCHDOG_TIMEOUT"] = config.WATCHDOG_TIMEOUT;

    doc["SD"]["SD_MONITOR_INTERVAL"] = config.SD_MONITOR_INTERVAL;
    doc["SD"]["SD_USAGE_THRESHOLD"] = config.SD_USAGE_THRESHOLD;


    serializeJson(doc, configFile);
    configFile.close();
    return true;
}
