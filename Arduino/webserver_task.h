/**
 * @file webserver_task.h
 * @brief Header for the Asynchronous Web Server task for fireCNC.
 *
 * Project: fireCNC
 * Version: 0.0.1
 * Author: Mark Dyer
 * Location: Blenheim, New Zealand
 * Contact: intelliservenz@gmail.com
 *
 * This file declares functions and includes for the web server, which
 * serves UI, displays health status graphs, and allows configuration changes.
 */
#ifndef WEBSERVER_TASK_H
#define WEBSERVER_TASK_H

#include "version.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>

/**
 * @brief Initializes and configures the Async Web Server.
 *
 * Sets up all routes, handlers, and the WebSocket server.
 */
void webserver_init();

/**
 * @brief The main FreeRTOS task for the web server.
 *
 * This task keeps the web server running and handles client connections.
 *
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void webserver_task(void* pvParameters);

#endif // WEBSERVER_TASK_H
