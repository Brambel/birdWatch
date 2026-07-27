/*
 * motion.h
 *
 *  Created on: Jul 27, 2026
 *      Author: James
 */

#ifndef MAIN_APP_HARDWARE_C4001_H_
#define MAIN_APP_HARDWARE_C4001_H_

#ifndef C4001_H
#define C4001_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#define C4001_UART_NUM  UART_NUM_1
#define C4001_TX_PIN    GPIO_NUM_1
#define C4001_RX_PIN    GPIO_NUM_0
#define C4001_BAUD_RATE 9600
#define C4001_BUF_SIZE  256

typedef enum {
    C4001_MODE_PRESENCE = 0,       
    C4001_MODE_SPEED_DISTANCE = 1 
} c4001_mode_t;

void C4001_Init(void);
void C4001_SetMode(c4001_mode_t mode);
c4001_mode_t C4001_GetMode(void);
void C4001_GetStatus(bool *detected, uint32_t *total_count);
void C4001_GetLatestData(bool *detected, uint32_t *total_count, 
                         float *distance, float *speed, int *energy, 
                         char *last_packet, size_t max_len, 
                         char *timestamp_buf, size_t ts_max_len);

#endif // C4001_H
#endif /* MAIN_APP_HARDWARE_MOTION_H_ */
