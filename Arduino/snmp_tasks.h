/**
 * @file snmp_tasks.h
 * @brief Header for SNMP agent and trap functions.
 *
 * Project: fireCNC
 * Version: 0.0.1
 * Author: Mark Dyer
 * Location: Blenheim, New Zealand
 * Contact: intelliservenz@gmail.com
 *
 * This file declares functions for initializing the SNMP agent, handling
 * custom SNMP variables, and sending SNMP traps.
 */
#ifndef SNMP_TASKS_H
#define SNMP_TASKS_H

#include "version.h"
#include <SNMP_Agent.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/**
 * @brief Initializes and starts the SNMP agent.
 *
 * This function sets up the SNMP agent with the configured community and
 * other parameters from the SD card.
 */
void snmp_init();

/**
 * @brief Sends an SNMP trap with a specified message.
 *
 * @param message The message to include in the trap payload.
 */
void snmp_trap_send(const String& message);

/**
 * @brief FreeRTOS task for the SNMP agent.
 *
 * This task runs the main SNMP agent loop, handling incoming requests
 * and managing the agent's state.
 *
 * @param pvParameters Standard FreeRTOS task parameters (not used).
 */
void snmp_agent_task(void* pvParameters);

#endif // SNMP_TASKS_H
