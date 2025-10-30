#include "ssh_tasks.h"
#include "config.h"
#include "sd_tasks.h"
#include "snmp_tasks.h"
#include <libssh_esp32.h>
#include <SD.h>

#define SSH_HOST_KEY_PATH "/ssh_host_rsa_key"
#define SSH_TASK_STACK_SIZE 8192
#define SSH_TASK_PRIORITY 2

static void ssh_server_task(void *arg);

// The SSH command handler function. It's a simple example.
int ssh_command_handler(const char *cmd, char *response_buffer, size_t buffer_size) {
    if (strcmp(cmd, "health") == 0) {
        snprintf(response_buffer, buffer_size, "System health is OK.\n");
    } else if (strcmp(cmd, "reboot") == 0) {
        snprintf(response_buffer, buffer_size, "Rebooting...\n");
        log_to_sd("SSH command: Reboot initiated.");
        delay(100);
        ESP.restart();
    } else if (strncmp(cmd, "echo ", 5) == 0) {
        snprintf(response_buffer, buffer_size, "%s\n", cmd + 5);
    } else {
        snprintf(response_buffer, buffer_size, "Unknown command: %s\n", cmd);
    }
    return strlen(response_buffer);
}

// The SSH authentication callback function.
bool ssh_auth_callback(const char *username, const char *password) {
    return (strcmp(username, config.SSH_USERNAME.c_str()) == 0) &&
           (strcmp(password, config.SSH_PASSWORD.c_str()) == 0);
}

void ssh_init() {
    // libssh_esp32 requires a FreeRTOS task to operate.
    xTaskCreate(ssh_server_task, "ssh_server_task", SSH_TASK_STACK_SIZE, NULL, SSH_TASK_PRIORITY, NULL);
}

static void ssh_server_task(void *arg) {
    libssh_begin();

    // Check for existing host key and generate if not present
    if (!SD.exists(SSH_HOST_KEY_PATH)) {
        log_to_sd("SSH host key not found, generating a new one.");
        // Note: Key generation can be resource-intensive.
        // It's often better to generate the key offline and store it.
        // For demonstration, we'll assume a key exists after first run.
        // Key generation logic (uncomment to use once):
        // libssh_generate_rsa_key(SSH_HOST_KEY_PATH, 2048);
        // if (!SD.exists(SSH_HOST_KEY_PATH)) {
        //     log_to_sd("Failed to generate SSH host key.");
        //     snmp_trap_send("SSH Host Key Generation Failed.");
        //     vTaskDelete(NULL);
        // }
    }

    libssh_server_set_host_key(SSH_HOST_KEY_PATH);
    libssh_server_set_auth_callback(ssh_auth_callback);
    libssh_server_set_command_callback(ssh_command_handler);
    libssh_server_start();

    // The SSH server runs indefinitely within this task.
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
