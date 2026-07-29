/*
 * tcp_server.c
 *
 *  Created on: Jul 27, 2026
 *      Author: james
 */

#include "tcp_server.h"

#include <string.h>
#include "esp_log.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define TAG "TCP"

#define SERVER_PORT 2323
#define RX_BUFFER_SIZE 256

static int clientSocket = -1;
static void (*rxCallback)(const uint8_t *, size_t) = NULL;

static const char BIRD_WATCH_BANNER[] =
    "\r\n"
    "______ _         _ _    _       _       _      \r\n"
    "| ___ (_)       | | |  | |     | |     | |     \r\n"
    "| |_/ /_ _ __ __| | |  | | __ _| |_ ___| |__   \r\n"
    "| ___ \\ | '__/ _` | |/\\| |/ _` | __/ __| '_ \\  \r\n"
    "| |_/ / | | | (_| \\  /\\  / (_| | || (__| | | | \r\n"
    "\\____/|_|_|  \\__,_|\\/  \\/ \\__,_|\\__\\___|_| |_| \r\n"
    "                                               \r\n"
    "                                               \r\n"
    "      .__---~~~(~~-_.\r\n"
    "    _-'  ) -~~- ) _-\" )_\r\n"
    "  (  ( `-,_..`.,_--_ '_,)_\r\n"
    " (  -_)  ( -_-~  -_ `,    )\r\n"
    " (_ -_ _-~-__-~`, ,' )__-'))--___--~~~--__--~~--___--__..\r\n"
    "  _ ~`_-'( (____;--==,,_))))--___--~~~--__--~~--__----~~~'`=__-~_-.\r\n"
    " / \\ / \\````      `-_(())_-~~--__..\r\n"
    "|O  |O |\r\n"
    " \\_/ \\_/\r\n\r\n\r\n";

static void TCP_Server_Drain(void)
{
    // Allow PuTTY / Telnet startup negotiation bytes to clear
    vTaskDelay(pdMS_TO_TICKS(100));

    // Non-blocking drain
    char dummy[128];
    while (recv(clientSocket, dummy, sizeof(dummy), MSG_DONTWAIT) > 0)
    {
        // Discard negotiation bytes
    }
}

static void TCP_Server_Task(void *arg)
{
    uint8_t rxBuffer[RX_BUFFER_SIZE];
    struct sockaddr_in serverAddr;

    int listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listenSocket < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        vTaskDelete(NULL);
        return;
    }

    // Allow immediate reuse of port on disconnect
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed");
        close(listenSocket);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listenSocket, 1) < 0) {
        ESP_LOGE(TAG, "Listen failed");
        close(listenSocket);
        vTaskDelete(NULL);
        return;
    }

    while (1)
    {
        ESP_LOGI(TAG, "Listening on port %d...", SERVER_PORT);

        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket < 0) {
            ESP_LOGE(TAG, "Accept failed");
            break;
        }

        ESP_LOGI(TAG, "Client Connected");
        TCP_Server_Drain();
		
		TCP_Server_Write(BIRD_WATCH_BANNER);
		TCP_Server_Write("birdWatch> ");

        while (1)
        {
            int len = recv(clientSocket, rxBuffer, sizeof(rxBuffer), 0);

            if (len <= 0)
            {
                ESP_LOGI(TAG, "Client Disconnected");
                close(clientSocket);
                clientSocket = -1;
                break;
            }

            // Route incoming TCP bytes to CLI callback
            if (rxCallback != NULL)
            {
                rxCallback(rxBuffer, (size_t)len);
            }
        }
    }

    close(listenSocket);
    vTaskDelete(NULL);
}

void TCP_Server_Init(void)
{
		
    xTaskCreate(
        TCP_Server_Task,
        "TCP_Task",
        4096,
        NULL,
        5,
        NULL);
		
		ESP_LOGD("TCP_SERVER","finished init");
}

void TCP_Server_Write(const char *str)
{
    if (clientSocket >= 0 && str != NULL)
    {
        send(clientSocket, str, strlen(str), 0);
    }
}

void TCP_Server_RegisterRxCallback(void (*callback)(const uint8_t *data, size_t len))
{
    rxCallback = callback;
}