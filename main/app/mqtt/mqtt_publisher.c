/*
 * mqtt_publisher.c
 *
 *  Created on: Aug 1, 2026
 *      Author: james
 */

 #include "mqtt_publisher.h"
 #include "app/configuration.h"
 #include "app/hardware/device_info.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/queue.h"
 #include "freertos/task.h"
 #include "mqtt_client.h"
 #include "esp_log.h"
 #include <stdio.h>

 #undef TAG 
 #define TAG "MQTT_PUB"

 #define EVENT_QUEUE_SIZE 50 

 //ESP_EVENT_DEFINE_BASE(BIRD_WATCH_EVENTS);
  
 static QueueHandle_t s_detection_queue = NULL;
 static esp_mqtt_client_handle_t s_mqtt_client = NULL;
 static bool s_mqtt_connected = false;

 // Event loop callback
 static void on_bird_detection(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
     detection_event_t *evt = (detection_event_t *)event_data;
     
     if (s_detection_queue != NULL) {
         if (xQueueSend(s_detection_queue, evt, 0) != pdTRUE) {
             detection_event_t dummy;
             xQueueReceive(s_detection_queue, &dummy, 0); 
             xQueueSend(s_detection_queue, evt, 0);       
             ESP_LOGW(TAG, "Queue full! Dropped oldest event # %u to store detection", (unsigned int)evt->count);
         }
     }
 }

 // Background Worker Task: Drains queue when connected
 static void mqtt_publisher_task(void *pvParameters) {
     detection_event_t evt;
     char payload[256];
     char topic[128];
     
     snprintf(topic, sizeof(topic), "%s/%s/%d/telemetry", 
              API_VERSION, SENSOR_TYPE, Device_GetIdNum());

     while (1) {
         if (xQueuePeek(s_detection_queue, &evt, portMAX_DELAY) == pdTRUE) {
             
             while (!s_mqtt_connected) {
                 vTaskDelay(pdMS_TO_TICKS(1000));
             }

             snprintf(payload, sizeof(payload),
                      "{\"deviceId\":%d,\"timestamp\":\"%s\",\"event\":\"%s\"}",
                      Device_GetIdNum(), evt.timestamp, "motion");

             int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, payload, 0, 1, 0);

             if (msg_id != -1) {
                 xQueueReceive(s_detection_queue, &evt, 0);
                 ESP_LOGI(TAG, "Published detection# %u, (Remaining in queue: %d)", 
                           (unsigned int)evt.count, uxQueueMessagesWaiting(s_detection_queue));
             } else {
                 ESP_LOGE(TAG, "Failed to publish event# %u retrying in 2s...", (unsigned int)evt.count);
                 vTaskDelay(pdMS_TO_TICKS(2000));
             }
         }
     }
 }

 static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
     esp_mqtt_event_handle_t event = event_data;
     switch ((esp_mqtt_event_id_t)event_id) {
         case MQTT_EVENT_CONNECTED:
             ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
             s_mqtt_connected = true;
             break;
         case MQTT_EVENT_DISCONNECTED:
             ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
             s_mqtt_connected = false;
             break;
         default:
             break;
     }
 }

 void MQTT_Publisher_Init(void) {
     s_detection_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(detection_event_t));

     esp_mqtt_client_config_t mqtt_cfg = {
         .broker.address.hostname = MQTT_ADDR,
         .broker.address.port = MQTT_PORT,
         .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
     };

     s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
     esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
     esp_mqtt_client_start(s_mqtt_client);

     ESP_ERROR_CHECK(esp_event_handler_instance_register(
         BIRD_WATCH_EVENTS, DETECTION_EVENT, &on_bird_detection, NULL, NULL));

     xTaskCreate(mqtt_publisher_task, "mqtt_pub_task", 4096, NULL, 5, NULL);
 }