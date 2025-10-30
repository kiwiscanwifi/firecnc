/**
 * @file fireCNC.ino
 * @brief Main file for the fireCNC project on the Waveshare ESP32-S3-POE-ETH-8DI-8DO.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file sets up the FreeRTOS tasks and orchestrates the different modules
 * for networking, LED control, servo communication, web server, and system monitoring.
 * All pin definitions have been moved to pins.h for better organization.
 */

#include "version.h"
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>
#include <ETH.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <Espalexa.h>
#include <ModbusMaster.h>
#include <Wire.h>
#include <TCA9554.h>
#include <esp_sleep.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <libssh_esp32.h>
#include <RingBuf.h>

// Include project-specific files
#include "pins.h"
#include "config.h"
#include "buzzer.h"
#include "networking.h"
#include "led_tasks.h"
#include "servo_tasks.h"
#include "webserver_task.h"
#include "snmp_tasks.h"
#include "sd_tasks.h"
#include "ssh_tasks.h"

// Global objects
CRGB* ledsY;
CRGB* ledsYY;
CRGB* ledsX;
ModbusMaster nodeY;
ModbusMaster nodeYY;
ModbusMaster nodeX;
TCA9554 tca9554;
Espalexa alexa;
AsyncWebServer server(80);
RingBuf<char, 2048> logBuffer;
volatile uint8_t alexa_brightness_y = 255;
volatile uint8_t alexa_brightness_yy = 255;
volatile uint8_t alexa_brightness_x = 255;

// FreeRTOS task handles
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t servoTaskHandle = NULL;
TaskHandle_t webserverTaskHandle = NULL;
TaskHandle_t sdMonitorTaskHandle = NULL;

// Alexa callback functions
void ledYBrightnessCallback(uint8_t brightness) {
    alexa_brightness_y = brightness;
}

void ledYYBrightnessCallback(uint8_t brightness) {
    alexa_brightness_yy = brightness;
}

void ledXBrightnessCallback(uint8_t brightness) {
    alexa_brightness_x = brightness;
}

void onboardLedCallback(uint8_t brightness) {
    analogWrite(ONBOARD_LED, brightness);
}

void shutdownCallback(uint8_t brightness) {
    if (brightness > 0) {
        shutdown_arduino();
    }
}

void chasingPurpleCallback(uint8_t brightness) {
    if (brightness > 0) {
        // Signal LED task to start chasing purple effect
        // (Implementation requires FreeRTOS queues or events)
    } else {
        // Signal LED task to stop chasing effect
    }
}

// System functions
void shutdown_arduino() {
    snmp_trap_send("Shutdown Initiated");
    beep(BUZZER_PIN, 2);
    crossfade_to_blue(5000); // Cross fade LED strips to blue over 5 seconds
    delay(1000); // Give a small delay after the animation
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    beep(BUZZER_PIN, 2); // Beep twice on power-up

    esp_reset_reason_t reason = esp_reset_reason();
    if (reason != ESP_RST_SW && reason != ESP_RST_WDT && reason != ESP_RST_DEEPSLEEP) {
        snmp_trap_send("Arduino Restarted. Reason: " + String(reason));
        log_to_sd("Arduino restarted. Reason: " + String(reason));
    }

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!tca9554.begin()) {
        snmp_trap_send("TCA9554 Initialization Failed");
        trigger_sd_error_visual();
        delay(5 * 60 * 1000);
        ESP.restart();
    }

    if (!SD.begin(SD_CMD_PIN, SPI, 4000000, "/sdcard", 1)) {
        trigger_sd_error_visual();
        delay(5 * 60 * 1000);
        ESP.restart();
    }
    snmp_trap_send("SD Card Loaded");
    beep(BUZZER_PIN, 1);

    if (!load_config_from_sd()) {
        trigger_sd_error_visual();
        delay(5 * 60 * 1000);
        ESP.restart();
    }
    snmp_trap_send("Config Loaded");
    beep(BUZZER_PIN, 1);

    ledsY = new CRGB[config.LEDS_Y_COUNT];
    ledsYY = new CRGB[config.LEDS_YY_COUNT];
    ledsX = new CRGB[config.LEDS_X_COUNT];
    FastLED.addLeds<WS2815, LEDY_PIN, GRB>(ledsY, config.LEDS_Y_COUNT).setCorrection(TypicalSMD5050);
    FastLED.addLeds<WS2815, LEDYY_PIN, GRB>(ledsYY, config.LEDS_YY_COUNT).setCorrection(TypicalSMD5050);
    FastLED.addLeds<WS2815, LEDX_PIN, GRB>(ledsX, config.LEDS_X_COUNT).setCorrection(TypicalSMD5050);

    alexa.addDevice("LEDY Brightness", ledYBrightnessCallback, EspalexaDeviceType::dimmable);
    alexa.addDevice("LEDYY Brightness", ledYYBrightnessCallback, EspalexaDeviceType::dimmable);
    alexa.addDevice("LEDX Brightness", ledXBrightnessCallback, EspalexaDeviceType::dimmable);
    alexa.addDevice("Onboard LED", onboardLedCallback, EspalexaDeviceType::dimmable);
    alexa.addDevice("Shutdown", shutdownCallback, EspalexaDeviceType::onoff);
    alexa.addDevice("Chasing Purple", chasingPurpleCallback, EspalexaDeviceType::onoff);
    alexa.begin();

    xTaskCreate(networking_task, "networking_task", 4096, NULL, 1, NULL);
    xTaskCreate(led_task, "led_task", 4096, NULL, 1, &ledTaskHandle);
    xTaskCreate(servo_task, "servo_task", 4096, NULL, 1, &servoTaskHandle);
    xTaskCreate(webserver_task, "webserver_task", 8192, NULL, 1, &webserverTaskHandle);
    xTaskCreate(sd_monitor_task, "sd_monitor_task", 4096, NULL, 1, &sdMonitorTaskHandle);
    ssh_init();

    esp_task_wdt_init(config.WATCHDOG_TIMEOUT, true);
    esp_task_wdt_add(ledTaskHandle);
    esp_task_wdt_add(servoTaskHandle);
    esp_task_wdt_add(webserverTaskHandle);
    esp_task_wdt_add(sdMonitorTaskHandle);
    esp_task_wdt_add(xTaskGetIdleTaskHandle(0));
    esp_task_wdt_add(xTaskGetIdleTaskHandle(1));
}

void loop() {
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
}
