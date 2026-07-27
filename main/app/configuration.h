/*
 * configuration.h
 *
 *  Created on: Jul 22, 2026
 *      Author: james
 */

#ifndef MAIN_CONFIGURATION_H_
#define MAIN_CONFIGURATION_H_

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#define VERSION_MAJOR      0
#define VERSION_MINOR      1
#define VERSION_PATCH      0

#define VERSION_STRING STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)


#define WIFI_SSID     "badDogs"
#define WIFI_PASSWORD "TheDogEatsEverything"

#define TAG "APP"
#define APP_LOG_LEVEL_GLOBAL   ESP_LOG_DEBUG

//Define specific tag log levels by tag
#define APP_TAG_LOG_LEVELS {               \
    { "APP",       ESP_LOG_DEBUG   }       \
}

#endif /* MAIN_CONFIGURATION_H_ */
