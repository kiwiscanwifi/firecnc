/**
 * @file networking.cpp
 * @brief Implementation of network connectivity functions and related tasks.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This module handles the logic for connecting to Ethernet, Wi-Fi, and falling back
 * to a static IP. It uses ESP-IDF event handlers to react to network events
 * and includes functions for updating the time via NTP.
 */
#include "networking.h"
#include "version.h"
#include "config.h"
#include "pins.h"
#include "sd_tasks.h"
#include "led_tasks.h"
#include "snmp_tasks.h"
#include <WiFi.h>
#include <ETH.h>

// Flags and state variables
bool last_connection_is_ethernet = true;
static bool ethernet_connected = false;
static bool wifi_connected = false;

// Function prototypes for internal use
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void init_network_stack();

/**
 * @brief Initializes the ESP-IDF network event handlers and starts the networking flow.
 */
static void init_network_stack() {
    log_to_sd("Initializing network stack...");
    
    // Register network event handlers
    esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, on_got_ip, NULL);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_got_ip, NULL);
}

/**
 * @brief Configures and starts the W5500 Ethernet interface.
 */
void start_ethernet() {
    log_to_sd("Attempting Ethernet connection...");
    SPI.begin(ETH_SPI_SCK, ETH_SPI_MISO, ETH_SPI_MOSI, ETH_SPI_CS);
    ETH.begin(ETH_SPI_CS, ETH_PHY_RST, ETH_PHY_INT);
}

/**
 * @brief Configures and starts the Wi-Fi connection.
 * 
 * @param use_static_ip True to use a static IP, False to use DHCP.
 */
void start_wifi(bool use_static_ip) {
    if (use_static_ip) {
        IPAddress local_ip, gateway, subnet, dns;
        local_ip.fromString(config.STATIC_IP);
        gateway.fromString(config.GATEWAY);
        subnet.fromString(config.SUBNET);
        dns.fromString(config.DNS_SERVER);
        WiFi.config(local_ip, subnet, gateway, dns);
        log_to_sd("Attempting Wi-Fi with static IP...");
    } else {
        log_to_sd("Attempting Wi-Fi with DHCP...");
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE); // Clear any previous static config
    }
    WiFi.begin(config.WIFI_SSID.c_str(), config.WIFI_PASSWORD.c_str());
}

/**
 * @brief Tries to connect to the network using the last known successful method.
 */
void reconnect_last_working() {
    if (last_connection_is_ethernet) {
        start_ethernet();
    } else {
        start_wifi(false);
    }
}

/**
 * @brief Updates the RTC using NTP.
 * 
 * First tries to get the NTP server from DHCP option 42,
 * falls back to the configured server if DHCP fails.
 */
void update_ntp() {
    sntp_servermode_dhcp(1); // Enable DHCP option 42 for NTP
    sntp_init();

    // Set fallback server and apply if DHCP doesn't provide
    sntp_setservername(0, config.NTP_SERVER.c_str());

    log_to_sd("Attempting NTP synchronization...");
    int retry_count = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_PENDING && retry_count < 10) {
        delay(1000);
        retry_count++;
    }

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%c", &timeinfo);
        log_to_sd("NTP synchronization successful. Time: " + String(time_str));
    } else {
        log_to_sd("NTP synchronization failed. Using fallback server or no time sync.");
    }
}

/**
 * @brief FreeRTOS task for managing network connectivity.
 */
void networking_task(void* pvParameters) {
    init_network_stack();

    while (1) {
        // Only attempt to connect if not already connected
        if (!ethernet_connected && !wifi_connected) {
            log_to_sd("Network disconnected. Attempting reconnection sequence.");
            
            // 1. Try last successful connection first
            if (last_connection_is_ethernet) {
                start_ethernet();
                vTaskDelay(pdMS_TO_TICKS(10000));
                if (!ethernet_connected) {
                    log_to_sd("Ethernet connection failed, trying Wi-Fi.");
                    start_wifi(false);
                    vTaskDelay(pdMS_TO_TICKS(15000));
                }
            } else {
                start_wifi(false);
                vTaskDelay(pdMS_TO_TICKS(15000));
                if (!wifi_connected) {
                    log_to_sd("Wi-Fi connection failed, trying Ethernet.");
                    start_ethernet();
                    vTaskDelay(pdMS_TO_TICKS(10000));
                }
            }

            // 2. Fallback to static IP if previous attempts failed
            if (!ethernet_connected && !wifi_connected) {
                log_to_sd("All dynamic connection methods failed. Falling back to static IP.");
                start_wifi(true);
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Wait before next check
    }
}

// Event handler for Ethernet events
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            log_to_sd("Ethernet Link Up");
            ethernet_connected = true;
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            log_to_sd("Ethernet Link Down");
            ethernet_connected = false;
            break;
        case ETHERNET_EVENT_START:
            log_to_sd("Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            log_to_sd("Ethernet Stopped");
            break;
        default:
            break;
    }
}

// Event handler for Wi-Fi events
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case WIFI_EVENT_STA_START:
            log_to_sd("Wi-Fi STA Started");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            log_to_sd("Wi-Fi Disconnected");
            wifi_connected = false;
            break;
        default:
            break;
    }
}

// Common event handler for IP acquisition
static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        last_connection_is_ethernet = true;
        ethernet_connected = true;
        log_to_sd("Ethernet connected with IP: " + ETH.localIP().toString());
        two_short_blue_flashes();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        last_connection_is_ethernet = false;
        wifi_connected = true;
        log_to_sd("Wi-Fi connected with IP: " + WiFi.localIP().toString());
        if (event->ip_info.ip.toString() == config.STATIC_IP) {
            green_flash(3000);
        } else {
            two_short_blue_flashes();
        }
    }
    snmp_trap_send("Network Connected");
    update_ntp();
}
