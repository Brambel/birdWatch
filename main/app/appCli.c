/*
 * appCli.c
 *
 *  Created on: Jul 27, 2026
 *      Author: james
 */

#include "appCli.h"

#include "app/hardware/C4001.h"
#include "app/transport/tcp_server.h"
#include "micro_cli_lib.h"

#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Formats text and sends it directly back over TCP to the connected CLI client
static void write_console(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    TCP_Server_Write(buf);
}

/**
* Define Command Table (NULL-terminated)
**/
static void Help_Command(int argc, char *argv[]);
static void Mode_Command(int argc, char *argv[]);

static const cli_command_t commandTable[] = {
    {"help", Help_Command, "Displays available commands"},
    {"mode", Mode_Command, "Get/set C4001 radar mode (0: Presence, 1: Speed/Distance)"},
    {NULL, NULL, NULL}
};

static void Help_Command(int argc, char *argv[]) {
    int i = 0;

    write_console("Available Commands\r\n");
    write_console("------------------\r\n");

    while (commandTable[i].name != NULL) {
        write_console("%s\t\t%s\r\n", commandTable[i].name, commandTable[i].help);
        i++;
    }
}

static void Mode_Command(int argc, char *argv[]) {
    if (argc < 2) {
        c4001_mode_t current = C4001_GetMode();
        write_console("Current C4001 Mode: %d (%s)\r\n", (int)current,
                      current == C4001_MODE_PRESENCE ? "Presence" : "Speed & Distance");
        write_console("Usage: mode <0|1>  (0 = Presence, 1 = Speed & Distance)\r\n");
        return;
    }

    int mode_val = atoi(argv[1]);
    if (mode_val == 0) {
        C4001_SetMode(C4001_MODE_PRESENCE);
        write_console("C4001 set to Mode 0 (Presence Mode)\r\n");
    } else if (mode_val == 1) {
        C4001_SetMode(C4001_MODE_SPEED_DISTANCE);
        write_console("C4001 set to Mode 1 (Speed & Distance Mode)\r\n");
    } else {
        write_console("Invalid mode '%s'. Use 0 (Presence) or 1 (Speed & Distance)\r\n", argv[1]);
    }
}

/**
 END Command Table
 */

static void cli_print_adapter(const char *str) 
{
    TCP_Server_Write(str);
}

static void cli_putc_adapter(char c) 
{
    char buf[2] = { c, '\0' };
    TCP_Server_Write(buf);
}

static void TCP_RX_Callback(const uint8_t *data, size_t len) 
{
    for (size_t i = 0; i < len; i++) 
    {
        char c = (char)data[i];
       
        // Handle carriage return / newline echo
        if ( c == '\n') {
            continue;
        }

        CLI_ProcessChar(c); 
    }
}

void Cli_Init(void) 
{
    cli_config_t cli_cfg = {
        .commands = commandTable,
        .print = cli_print_adapter,
        .putc = cli_putc_adapter,
        .prompt = "\r\nbirdWatch> "
    };

    CLI_Init(&cli_cfg);

    // Wire up incoming TCP data directly to callback
    TCP_Server_RegisterRxCallback(TCP_RX_Callback);
    
    ESP_LOGD("APP_CLI", "finished init");
}