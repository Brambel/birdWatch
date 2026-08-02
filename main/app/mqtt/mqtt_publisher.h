/*
 * mqtt_publisher.h
 *
 *  Created on: Aug 1, 2026
 *      Author: james
 */

#ifndef MAIN_APP_MQTT_MQTT_PUBLISHER_H_
#define MAIN_APP_MQTT_MQTT_PUBLISHER_H_

#include "esp_event.h"

// Declare the event base
ESP_EVENT_DECLARE_BASE(BIRD_WATCH_EVENTS);

typedef enum {
    DETECTION_EVENT,
} mqtt_event_id_t;

typedef struct {
    uint32_t count;
    char timestamp[32];
} detection_event_t;

void MQTT_Publisher_Init();

#endif /* MAIN_APP_MQTT_MQTT_PUBLISHER_H_ */
