/*
 * C4001.c
 *
 *  Created on: Jul 27, 2026
 *      Author: James
 */

#include "c4001.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "C4001";

static bool s_is_detected = false;
static uint32_t s_detection_count = 0;
static float s_distance_m = 0.0f;
static float s_speed_ms = 0.0f;
static int s_energy = 0;

static char s_last_raw_packet[128] = "None";
static char s_last_timestamp[32] = "N/A";
static SemaphoreHandle_t s_status_mutex = NULL;
static QueueHandle_t s_uart_queue;
static c4001_mode_t s_current_mode = C4001_MODE_PRESENCE;

c4001_mode_t C4001_GetMode(void) {
    return s_current_mode;
}

static void send_uart_cmd(const char *cmd) {
    uart_write_bytes(C4001_UART_NUM, cmd, strlen(cmd));
    vTaskDelay(pdMS_TO_TICKS(150));
}

void C4001_SetMode(c4001_mode_t mode) {
    s_current_mode = mode;
    ESP_LOGI(TAG, "Configuring C4001 to Mode %d (%s)...", (int)mode,
             mode == C4001_MODE_PRESENCE ? "Presence" : "Speed & Distance");
    
    send_uart_cmd("sensorStop\r\n");
    if (mode == C4001_MODE_PRESENCE) {
        send_uart_cmd("setRunApp 0\r\n");
    } else {
        send_uart_cmd("setRunApp 1\r\n");
    }
    send_uart_cmd("saveConfig\r\n");
    send_uart_cmd("sensorStart\r\n");
}

static void get_uptime_timestamp(char *buf, size_t max_len) {
    int64_t total_sec = esp_timer_get_time() / 1000000;
    int hours = (int)(total_sec / 3600);
    int mins = (int)((total_sec % 3600) / 60);
    int secs = (int)(total_sec % 60);
    snprintf(buf, max_len, "%02d:%02d:%02d", hours, mins, secs);
}

void C4001_GetStatus(bool *detected, uint32_t *total_count) {
    if (s_status_mutex && xSemaphoreTake(s_status_mutex, pdMS_TO_TICKS(50))) {
        if (detected) *detected = s_is_detected;
        if (total_count) *total_count = s_detection_count;
        xSemaphoreGive(s_status_mutex);
    }
}

void C4001_GetLatestData(bool *detected, uint32_t *total_count, 
                         float *distance, float *speed, int *energy, 
                         char *last_packet, size_t max_len, 
                         char *timestamp_buf, size_t ts_max_len) {
    if (s_status_mutex && xSemaphoreTake(s_status_mutex, pdMS_TO_TICKS(50))) {
        if (detected) *detected = s_is_detected;
        if (total_count) *total_count = s_detection_count;
        if (distance) *distance = s_distance_m;
        if (speed) *speed = s_speed_ms;
        if (energy) *energy = s_energy;
        if (last_packet) strncpy(last_packet, s_last_raw_packet, max_len - 1);
        if (timestamp_buf) strncpy(timestamp_buf, s_last_timestamp, ts_max_len - 1);
        xSemaphoreGive(s_status_mutex);
    }
}

// Parses $DFHPD or $DFDMD sentences
static void parse_c4001_line(const char *line) {
    bool current_state = false;
    bool valid_packet = false;
    float temp_dist = 0.0f;
    float temp_speed = 0.0f;
    int temp_energy = 0;

    // Parse Mode 0 ($DFHPD) Presence Packets
    if (strncmp(line, "$DFHPD,", 7) == 0) {
        int presence = 0;
        if (sscanf(line, "$DFHPD,%d", &presence) == 1) {
            current_state = (presence > 0);
            valid_packet = true;
        }
    } 
    // Parse Mode 1 ($DFDMD) Distance & Speed Packets
    else if (strncmp(line, "$DFDMD,", 7) == 0) {
        int motion = 0, presence = 0;
        if (sscanf(line, "$DFDMD,%d,%d,%f,%f,%d", &motion, &presence, &temp_dist, &temp_speed, &temp_energy) >= 4) {
            current_state = (motion > 0 || presence > 0);
            valid_packet = true;
        }
    }

    // Process State Update safely under mutex
    if (valid_packet && s_status_mutex && xSemaphoreTake(s_status_mutex, pdMS_TO_TICKS(50))) {
        s_distance_m = temp_dist;
        s_speed_ms = temp_speed;
        s_energy = temp_energy;

        if (current_state && !s_is_detected) {
            s_detection_count++;
            get_uptime_timestamp(s_last_timestamp, sizeof(s_last_timestamp));
            ESP_LOGI(TAG, "[%s] >>> MOTION DETECTED (Mode %d)! Total: %" PRIu32 " <<<", 
                     s_last_timestamp, (int)s_current_mode, s_detection_count);
        } else if (!current_state && s_is_detected) {
            ESP_LOGI(TAG, "Target cleared.");
        }

        if (current_state) {
            snprintf(s_last_raw_packet, sizeof(s_last_raw_packet), "%s", line);
        }

        s_is_detected = current_state;
        xSemaphoreGive(s_status_mutex);
    }
}

static void c4001_uart_event_task(void *pvParameters) {
    uart_event_t event;
    uint8_t dtmp[128];

    // Synchronize sensor hardware to initial mode on boot
    C4001_SetMode(s_current_mode);

    char rx_line[128];
    uint16_t rx_idx = 0;

    while (1) {
        if (xQueueReceive(s_uart_queue, (void *)&event, portMAX_DELAY)) {
            if (event.type == UART_DATA) {
                int len = uart_read_bytes(C4001_UART_NUM, dtmp, event.size, portMAX_DELAY);
                
                for (int i = 0; i < len; i++) {
                    char c = (char)dtmp[i];

                    if (c == '\n') {
                        rx_line[rx_idx] = '\0';
                        parse_c4001_line(rx_line);
                        rx_idx = 0;
                    } 
                    else if (c != '\r' && rx_idx < (sizeof(rx_line) - 1)) {
                        rx_line[rx_idx++] = c;
                    }
                }
            } 
            else if (event.type == UART_FIFO_OVF || event.type == UART_BUFFER_FULL) {
                uart_flush_input(C4001_UART_NUM);
                xQueueReset(s_uart_queue);
                rx_idx = 0;
            }
        }
    }

    vTaskDelete(NULL);
}

void C4001_Init(void) {
    s_status_mutex = xSemaphoreCreateMutex();

    const uart_config_t uart_config = {
        .baud_rate = C4001_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(C4001_UART_NUM, C4001_BUF_SIZE * 2, C4001_BUF_SIZE * 2, 20, &s_uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(C4001_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(C4001_UART_NUM, C4001_TX_PIN, C4001_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    gpio_pullup_en(C4001_RX_PIN);

    ESP_LOGI(TAG, "UART initialized on RX: GPIO%d, TX: GPIO%d", C4001_RX_PIN, C4001_TX_PIN);

    xTaskCreate(c4001_uart_event_task, "c4001_event_task", 4096, NULL, 12, NULL);
}