/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "app/configuration.h"
#include "app/appCli.h"
#include "app/hardware/c4001.h"
#include "app/transport/tcp_server.h"
#include "app/network/webserver.h"
#include "app/network/wifi.h"

#include "esp_log.h"
#include "freertos/idf_additions.h"

typedef struct {
    const char *tag;
    esp_log_level_t level;
} app_log_config_t;

void App_Log_Init(void)
{
    // Set the default global level
    esp_log_level_set("*", APP_LOG_LEVEL_GLOBAL);

    // Apply tag-specific log levels from macro table
    const app_log_config_t log_configs[] = APP_TAG_LOG_LEVELS;
    size_t count = sizeof(log_configs) / sizeof(log_configs[0]);

    for (size_t i = 0; i < count; i++) {
        esp_log_level_set(log_configs[i].tag, log_configs[i].level);
    }
	
	ESP_LOGI(TAG,"overall logging set to esp_log_level_t: %d", (int)APP_LOG_LEVEL_GLOBAL);
	
}

void app_main(void)
{
	
	App_Log_Init();
	
	
	WiFi_Init();
	WebServer_Init();
	TCP_Server_Init();
	C4001_Init();
	Cli_Init();
	ESP_LOGD(TAG,"finished init");
	
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

    
}
