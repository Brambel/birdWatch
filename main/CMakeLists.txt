idf_component_register(
	SRCS 
		main.c
		app/appCli.c
		app/hardware/c4001.c
		app/hardware/device_info.c
		app/mqtt/mqtt_publisher.c
		app/network/webserver.c
		app/network/wifi.c
		app/transport/uart_bridge.c
		app/transport/tcp_server.c
	PRIV_REQUIRES spi_flash
    INCLUDE_DIRS "."
    REQUIRES 
		micro_cli_lib
		esp_driver_uart
		esp_driver_gpio
		freertos
		esp_wifi
    	esp_http_server
    	esp_netif
    	esp_event
    	esp_timer
    	lwip
    	nvs_flash
    	mdns
    	espressif__mqtt
)
