/**
 * @file pins.h
 * @brief Centralized pin definitions for the fireCNC project.
 *
 * Project: fireCNC
 * Version: 1.0.0
 *
 * This header file consolidates all GPIO assignments and pin-related
 * configurations for the Waveshare ESP32-S3-POE-ETH-8DI-8DO board.
 * All other files should include this header to ensure consistent and
 * easily manageable pin assignments.
 */
#ifndef PINS_H
#define PINS_H

#include "version.h"

// LED Strip Data Pins
#define LEDY_PIN            1
#define LEDYY_PIN           2
#define LEDX_PIN            3

// Onboard Peripherals
#define BUZZER_PIN          46      // Onboard buzzer
#define ONBOARD_LED         38      // Onboard LED (e.g., WS2812B)

// Ethernet and W5500 SPI Interface Pins
// Using SPI2_HOST for the W5500 controller
#define ETH_SPI_HOST        SPI2_HOST
#define ETH_SPI_SCK         15
#define ETH_SPI_MISO        14
#define ETH_SPI_MOSI        13
#define ETH_SPI_CS          16
#define ETH_PHY_RST         39
#define ETH_PHY_INT         12

// RS485 Interface Pins (Using UART2)
#define RS485_TX_PIN        17
#define RS485_RX_PIN        18
#define RS485_RTS_PIN       21

// I2C Interface Pins (for TCA9554 and RTC)
#define I2C_SDA             42
#define I2C_SCL             41

// SD Card SPI Interface Pins
// Using SPI_HOST (default VSPI) for the SD card
#define SD_SPI_HOST         SPI_HOST
#define SD_CMD_PIN          47
#define SD_SCK_PIN          48
#define SD_D0_PIN           45

// Digital Input (DI) Pins on TCA9554 I/O Expander
// These are external inputs connected to the TCA9554, not direct ESP32 GPIOs.
// Mapped to the TCA9554's virtual pin numbers 0-7.
#define INPUT_DI01          4
#define INPUT_DI02          5
#define INPUT_DI03          6
#define INPUT_DI04          7
#define INPUT_DI05          8
#define INPUT_DI06          9
#define INPUT_DI07          10
#define INPUT_DI08          11

// External Digital Output (EXIO) Pins on TCA9554 I/O Expander
// Mapped to the TCA9554's virtual pin numbers 0-7.
#define OUTPUT_EXIO1        0
#define OUTPUT_EXIO2        1
#define OUTPUT_EXIO3        2
#define OUTPUT_EXIO4        3
#define OUTPUT_EXIO5        4
#define OUTPUT_EXIO6        5
#define OUTPUT_EXIO7        6
#define OUTPUT_EXIO8        7

#endif // PINS_H
