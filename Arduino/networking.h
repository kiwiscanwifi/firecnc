/**
 * @file networking.h
 * @brief Header for network connectivity functions and related tasks.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This file declares the network-related functions, including the FreeRTOS task
 * for managing connections (Ethernet, Wi-Fi, Static IP), event handlers, and
 * the NTP client.
 */
#ifndef NETWORKING_H
#define NETWORKING_H

#include "version.h"
#include <WiFi.h>
#include <ETH.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Variable to store the last successful connection type
extern bool last_connection_is_ethernet;

/**
 * @brief FreeRTOS task for handling all network connectivity.
 * 
 * This task manages the connection attempts for Ethernet, Wi-Fi, and static IP fallback.
 * It also handles NTP synchronization and network status visualization via LEDs.
 * 
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void networking_task(void* pvParameters);

/**
 * @brief Updates the RTC using NTP.
 * 
 * Configures and synchronizes the system time with an NTP server, first trying
 * the DHCP-provided server and then falling back to the configured default.
 */
void update_ntp();

#endif // NETWORKING_H
