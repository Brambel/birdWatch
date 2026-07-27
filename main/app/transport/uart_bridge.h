/*
 * uart_bridge.h
 *
 *  Created on: Jul 22, 2026
 *      Author: james
 */

#ifndef MAIN_UART_BRIDGE_H_
#define MAIN_UART_BRIDGE_H_

#include <stdint.h>

void UART_Bridge_Init(void);

void UART_Bridge_Update(void);

void UART_Bridge_Write(const char *str);

void UART_Bridge_Write_Char(char c);

void UART_Bridge_RegisterRxCallback( 
		void (*callback)(const uint8_t *data, size_t len)
	);

#endif /* MAIN_UART_BRIDGE_H_ */
