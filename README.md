Key features of the Arduino project
- Multi-threading: Utilizes ESP-IDF FreeRTOS capabilities by defining and implementing multiple tasks (vNetworkTask, vLEDTask, vServoTask, etc.) to handle different project aspects concurrently.
- Networking: Manages Ethernet and Wi-Fi connections with configurable fallback to a static IP address. Implements a robust reconnection logic. Integrates NTP sync via DHCP or a configurable server.
- Hardware: Controls WS2815 LED strips via the FastLED library and integrates an external I/O expander (TCA9554) and an onboard buzzer.
- Communication: Incorporates RS485 Modbus for communication with LC10e servo drivers. Also includes SNMP and SSH for remote management.
- Peripherals: Supports SD card operations for configuration, web pages, and logging. The SD card can be formatted via the web interface.
- User Interaction: Provides an asynchronous web server for monitoring and configuration, and integrates Espalexa for voice control.
- Robustness: Implements a task watchdog timer for system reliability. Includes comprehensive error handling for SD card, TCA9554, and network issues, with visual, audible, and SNMP alerts.
- Visual Effects: Features a range of LED effects, including startup effects, position tracking, limit switch alerts, idle dimming, and periodic chasing effects.
- Configuration: Reads and writes settings from a JSON file on the SD card, with a web interface for easy modification.
- Security: Provides SSH server functionality with configurable credentials.
- Safety: Includes an emergency shutdown sequence triggered by a dedicated GPIO pin.

