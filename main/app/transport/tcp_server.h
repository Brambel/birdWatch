/*
 * tcp_server.h
 *
 *  Created on: Jul 27, 2026
 *      Author: James
*/

#ifndef MAIN_TCP_SERVER_H_
#define MAIN_TCP_SERVER_H_

#include <stddef.h>
#include <stdint.h>

void TCP_Server_Init(void);
void TCP_Server_Write(const char *str);
void TCP_Server_RegisterRxCallback(void (*callback)(const uint8_t *data, size_t len));

#endif /* MAIN_TCP_SERVER_H_ */
