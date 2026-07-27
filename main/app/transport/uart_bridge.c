/*
 * uart_bridge.c
 *
 *  Created on: Jul 22, 2026
 *      Author: James
 */

#include "uart_bridge.h"

#include "app/configuration.h"

#include "driver/uart.h"
#include "driver/gpio.h"  //needed for define
#include "esp_log.h"

#undef TAG
#define TAG "UART"

#define UART_PORT UART_NUM_1

#define UART_TX_PIN GPIO_NUM_21
#define UART_RX_PIN GPIO_NUM_20

#define BUF_SIZE 1024

static uint8_t rxBuffer[BUF_SIZE];

static void (*rxCallback)(const uint8_t *, size_t) = NULL;

void UART_Bridge_Init(void)
{
    uart_config_t config =
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(
         uart_param_config(
             UART_PORT,
             &config));

    ESP_ERROR_CHECK(
        uart_driver_install(
            UART_PORT,
            BUF_SIZE,
            BUF_SIZE, //we may want a 0 buff size in the future
            0,
            NULL,
            0));

    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            UART_TX_PIN,
            UART_RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART Ready");
}

void UART_Bridge_Update(void)
{
    int len =
        uart_read_bytes(
            UART_PORT,
            rxBuffer,
            sizeof(rxBuffer),
            pdMS_TO_TICKS(30));

    if(len > 0)
    {
		if(rxCallback)
		{
		    rxCallback(rxBuffer, len);
		}
    }
}

void UART_Bridge_Write(const char *str)
{
    if (str != NULL) {
        uart_write_bytes(UART_PORT, str, strlen(str));
    }
}

void UART_Bridge_Write_Char(char c)
{
	uart_write_bytes( UART_PORT, (uint8_t *)&c, 1);
}

void UART_Bridge_RegisterRxCallback(void (*callback)( const uint8_t *data, size_t len))
{
    rxCallback = callback;
}
