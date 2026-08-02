/*
 * device_info.c
 *
 *  Created on: Aug 1, 2026
 *      Author: james
 */


 #include "device_info.h"
 #include "esp_mac.h"
 #include "esp_log.h"
 #include <stdio.h>

 static const char *TAG = "DEV_INFO";

 static char s_device_id_str[13] = "UNKNOWN";
 static uint32_t s_device_id_num = 0;

 void Device_Info_Init(void) {
     uint8_t mac[6];
     
     // Read base MAC address from eFuse
     esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);
     if (ret == ESP_OK) {
         // Format as uppercase hex string: "A4CF12345678"
         snprintf(s_device_id_str, sizeof(s_device_id_str), 
                  "%02X%02X%02X%02X%02X%02X",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

         // Optional: Use the lower 3 bytes as a unique 24-bit integer ID
         s_device_id_num = ((uint32_t)mac[3] << 16) | ((uint32_t)mac[4] << 8) | mac[5];

         ESP_LOGI(TAG, "Device ID Initialized -> String: %s, Numeric: %" PRIu32, 
                  s_device_id_str, s_device_id_num);
     } else {
         ESP_LOGE(TAG, "Failed to read base MAC address from eFuse!");
     }
 }

 int Device_GetIdNum(void) {
     return s_device_id_num;
 }

