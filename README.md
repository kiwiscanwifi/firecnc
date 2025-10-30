
![fireCNC - Learning Project for my installation of linuxCNC](Images/fireCNC-github-banner.jpg)

# fireCNC 
After experiencing a kitchen fire, I decided to rebuild my kitchen with the precision that CNC technology can provide. My hand tool skills are lacking and I struggle to cut a straight line with a hand saw, so I knew I needed a more accurate approach.

I've started gathering parts for the build, but financial constraints are slowing me down. In the meantime, I've begun assembling some of the electronics for the control boxes to keep the project moving forward.


## fireCNC Arduino ESP32 project

Configuration: The project uses a config.json file stored on an SD card, allowing easy customization of network settings, LED parameters, servo IDs, and other operational values without recompiling the code. 

### Key features 
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

### Project file structure

The Arduino code is designed to be split into multiple files (.ino, .h, and .cpp) to be organized and maintainable.
This approach adheres to standard best practices for projects of this complexity and ensures clarity.

- fireCNC is organized into several files, each responsible for a specific module, all adhering to the specified header format with author details and version number.
- version.h: Defines the project name and version.
- fireCNC.ino: The main Arduino file, containing setup() and loop().
- pins.h: Centralized definitions for all GPIO pins.
- config.h/config.cpp: Manages loading and saving configuration from config.json.
- networking.h/networking.cpp: Handles network connections (Ethernet, Wi-Fi, Static IP) and NTP.
- led_tasks.h/led_tasks.cpp: Manages all LED animations and effects.
- servo_tasks.h/servo_tasks.cpp: Handles RS485 communication with servos.
- webserver_task.h/webserver_task.cpp: Implements the asynchronous web server.
- snmp_tasks.h/snmp_tasks.cpp: Manages the SNMP agent and traps.
- sd_tasks.h/sd_tasks.cpp: Handles SD card logging and monitoring.
- ssh_tasks.h/ssh_tasks.cpp: Manages the SSH server.
- buzzer.h/buzzer.cpp: Utility functions for the onboard buzzer.

## Warning
There are quite a few mistakes and errors, and this is definitely a work in progress that may never reach completion.
