/*
* wifi.c
*
*  Created on: Jul 22, 2026
*      Author: James
*/

#include "wifi.h"
#include "app/configuration.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/projdefs.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "mdns.h"

#define WIFI_CONNECTED_BIT BIT0  //bit flag for blocking group

#undef TAG
static const char *TAG = "WIFI";
static EventGroupHandle_t s_wifi_event_group;

static void start_mdns_service(void)
{
    ESP_LOGI(TAG, "Initializing mDNS...");

    // initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
	
	//set host and instance name
    ESP_ERROR_CHECK(mdns_hostname_set(HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set(HOSTNAME " CLI"));

	ESP_ERROR_CHECK(mdns_service_add(
        HOSTNAME " CLI",   
        "_sensor",        
        "_tcp",            // Transport protocol
        TCP_SERVER_PORT,          
        NULL,             
        0                  
    ));
    ESP_LOGI(TAG, "mDNS initialized. Hostname: " HOSTNAME);
}

static void wifi_event_handler(
        void *arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void *event_data)
{
	
    if(event_base == WIFI_EVENT &&
       event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }

    if(event_base == WIFI_EVENT &&
       event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGW(TAG,"Disconnected");

        esp_wifi_connect();
    }

    if(event_base == IP_EVENT &&
       event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
		
		start_mdns_service();
		
	 	xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void WiFi_Init(void)
{
	//init blocking group
	s_wifi_event_group = xEventGroupCreate();
	
	
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(
        esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg =
        WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(
        esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL));

    wifi_config_t wifi_config = {0};

    strcpy((char *)wifi_config.sta.ssid,
           WIFI_SSID);

    strcpy((char *)wifi_config.sta.password,
           WIFI_PASSWORD);

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config));

    ESP_ERROR_CHECK(
        esp_wifi_start());

	// Block until WIFI_CONNECTED_BIT is set by the event handler
    xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );
		
    ESP_LOGI(TAG,"WiFi Started");
	ESP_LOGD(TAG,"finished init");
}


