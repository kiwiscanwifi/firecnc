/**
 * @file webserver_task.cpp
 * @brief Implementation of the Asynchronous Web Server task for fireCNC.
 *
 * Project: fireCNC
 * Version: 0.0.1
 * Author: Mark Dyer
 * Location: Blenheim, New Zealand
 * Contact: intelliservenz@gmail.com
 *
 * This module sets up and runs the web server using ESPAsyncWebServer.
 * It serves static files from the SD card, provides API endpoints for
 * health data, and includes functionality for configuration updates and restart.
 */
#include "webserver_task.h"
#include "version.h"
#include "config.h"
#include "networking.h"
#include "sd_tasks.h"
#include "pins.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Async Web Server on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Store power and voltage data for graphs (for demonstration)
// In a real application, this data would likely be stored in an array or file.
std::vector<float> power_data;
std::vector<float> voltage_data;

// Semaphore for data access protection
SemaphoreHandle_t dataMutex;

/**
 * @brief Handles WebSocket events.
 * @param server The WebSocket server instance.
 * @param client The client that triggered the event.
 * @param type The type of event.
 * @param arg Additional argument for the event.
 * @param data The data payload.
 * @param len The length of the data payload.
 */
void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        log_to_sd("WebSocket client connected.");
    } else if (type == WS_EVT_DISCONNECT) {
        log_to_sd("WebSocket client disconnected.");
    }
}

/**
 * @brief Retrieves all system health data as a JSON object.
 *
 * @param request The server request object.
 */
void handleDataRequest(AsyncWebServerRequest* request) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        DynamicJsonDocument doc(2048);
        doc["uptime"] = millis();
        doc["voltage"] = voltage_data.empty() ? 0 : voltage_data.back();
        doc["power"] = power_data.empty() ? 0 : power_data.back();
        doc["sd_total"] = (double)SD.cardSize();
        doc["sd_used"] = (double)SD.usedBytes();
        doc["sd_free_percent"] = (float)(SD.cardSize() - SD.usedBytes()) / SD.cardSize() * 100.0;

        JsonArray power_array = doc.createNestedArray("power_history");
        for (float p : power_data) {
            power_array.add(p);
        }

        JsonArray voltage_array = doc.createNestedArray("voltage_history");
        for (float v : voltage_data) {
            voltage_array.add(v);
        }

        String json_response;
        serializeJson(doc, json_response);
        request->send(200, "application/json", json_response);
        xSemaphoreGive(dataMutex);
    } else {
        request->send(503, "text/plain", "Server busy. Try again.");
    }
}

/**
 * @brief Handles the configuration update form.
 *
 * @param request The server request object.
 */
void handleConfigUpdate(AsyncWebServerRequest* request) {
    if (request->method() == HTTP_POST) {
        // Update network configuration
        if (request->hasParam("static_ip", true)) {
            config.STATIC_IP = request->getParam("static_ip", true)->value();
        }
        if (request->hasParam("gateway", true)) {
            config.DEFAULT_GATEWAY = request->getParam("gateway", true)->value();
        }
        if (request->hasParam("subnet", true)) {
            config.DEFAULT_SUBNET = request->getParam("subnet", true)->value();
        }
        if (request->hasParam("dns", true)) {
            config.DEFAULT_DNS = request->getParam("dns", true)->value();
        }
        
        save_config_to_sd();

        request->send(200, "text/plain", "Configuration updated. Restarting...");
        // Wait a moment for the response to be sent before restarting
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP.restart();
    } else {
        request->send(405, "text/plain", "Method Not Allowed");
    }
}

/**
 * @brief Handles the restart request.
 *
 * @param request The server request object.
 */
void handleRestart(AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Restarting...");
    // Wait a moment for the response to be sent before restarting
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
}

/**
 * @brief Initializes and configures the Async Web Server.
 */
void webserver_init() {
    dataMutex = xSemaphoreCreateMutex();
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Serve static files from SD card /www directory
    server.serveStatic("/", SD, "/www/").setDefaultFile("index.html");

    // Route for health data API
    server.on("/data", HTTP_GET, handleDataRequest);
    
    // Route for configuration update
    server.on("/config", HTTP_POST, handleConfigUpdate);
    
    // Route for restarting the ESP32
    server.on("/restart", HTTP_POST, handleRestart);

    // Start the server
    server.begin();
    log_to_sd("Web server started.");

    // Initialize ADC for ADC voltage readings
    // Use the pin from the loaded configuration
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)digitalPinToAnalogChannel(VOLTAGE_MONITORING_PIN), ADC_ATTEN_DB_11); 

}

/**
 * @brief Periodically collects data and sends it via WebSocket.
 */
void webserver_data_update() {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Collect current data (e.g., ADC voltage)
        // Use the pin from the loaded configuration
        float current_voltage = (float)analogRead(VOLTAGE_MONITORING_PIN) / 4095.0 * 3.3;
        float current_power = current_voltage * 0.5; // Example power calculation
        
        voltage_data.push_back(current_voltage);
        if (voltage_data.size() > 24 * 60) { // Keep 24 hours of data (1 minute intervals)
            voltage_data.erase(voltage_data.begin());
        }

        power_data.push_back(current_power);
        if (power_data.size() > 24 * 60) {
            power_data.erase(power_data.begin());
        }

        // Create and send JSON via WebSocket
        DynamicJsonDocument doc(1024);
        doc["voltage"] = current_voltage;
        doc["power"] = current_power;
        String json_payload;
        serializeJson(doc, json_payload);
        ws.textAll(json_payload);
        
        xSemaphoreGive(dataMutex);
    }
}

/**
 * @brief The main FreeRTOS task for the web server.
 */
void webserver_task(void* pvParameters) {
    webserver_init();
    while (1) {
        // Periodically update and send data via WebSocket
        webserver_data_update();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Update every 1 minute
    }
}
