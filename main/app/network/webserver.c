/*
 * webserver.c
 *
 *  Created on: Jul 27, 2026
 *      Author: James
*/

#include "webserver.h"

#include "app/hardware/c4001.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include <inttypes.h> // For PRIu32 macro

static const char *TAG = "WEBSERVER";

static esp_err_t StatusHandler(httpd_req_t *req)
{
    bool detected = false;
    uint32_t count = 0;
    float distance = 0.0f;
    float speed = 0.0f;
    int energy = 0;
    char raw_packet[128] = {0};
    char timestamp[32] = {0};

    C4001_GetLatestData(&detected, &count, &distance, &speed, &energy, 
                         raw_packet, sizeof(raw_packet), 
                         timestamp, sizeof(timestamp));

    char json_response[300];
    snprintf(json_response, sizeof(json_response), 
             "{\"detected\":%s,\"count\":%" PRIu32 ",\"distance\":%.2f,\"speed\":%.2f,\"energy\":%d,\"last_time\":\"%s\",\"raw\":\"%s\"}", 
             detected ? "true" : "false", count, distance, speed, energy, timestamp, raw_packet);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
}

// 2. Index Handler
static esp_err_t IndexHandler(httpd_req_t *req)
{
    static const char html[] =
        "<!DOCTYPE html><html><head>"
        "<link rel=\"icon\" type=\"image/svg+xml\" href=\"/favicon.ico\">"
        "<title>BirdWatch Target Monitor</title>"
        "<style>"
        "  body { font-family: sans-serif; text-align: center; margin-top: 50px; background: #121212; color: #eee; }"
        "  .status-card { display: inline-block; padding: 20px 40px; border-radius: 12px; background: #222; }"
        "  .active { color: #00ff66; font-size: 24px; font-weight: bold; }"
        "  .idle { color: #888; font-size: 24px; }"
        "</style>"
        "</head><body>"
        "  <div class=\"status-card\">"
        "    <h1>BirdWatch</h1>"
        "    <p>Target Presence: <span id=\"status\" class=\"idle\">IDLE</span></p>"
        "    <p>Total Detections: <span id=\"count\">0</span></p>"
        "  </div>"
        "<script>"
        "  setInterval(() => {"
        "    fetch('/status')"
        "      .then(res => res.json())"
        "      .then(data => {"
        "        const el = document.getElementById('status');"
        "        document.getElementById('count').innerText = data.count;"
        "        if (data.detected) {"
        "          el.innerText = 'MOTION DETECTED';"
        "          el.className = 'active';"
        "        } else {"
        "          el.innerText = 'CLEAR';"
        "          el.className = 'idle';"
        "        }"
        "      });"
        "  }, 500);"
        "</script>"
        "</body></html>";

    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 3. Favicon Handler
static const char favicon_svg[] = 
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\">"
    "<text y=\".9em\" font-size=\"90\">🐦</text>"
    "</svg>";

static esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=86400");
    return httpd_resp_send(req, favicon_svg, HTTPD_RESP_USE_STRLEN);
}

void register_favicon(httpd_handle_t server) {
    httpd_uri_t favicon_uri = {
        .uri      = "/favicon.ico",
        .method   = HTTP_GET,
        .handler  = favicon_handler
    };
    httpd_register_uri_handler(server, &favicon_uri);
}

// 4. WebServer Init
void WebServer_Init(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        
        // Route /
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = IndexHandler
        };
        httpd_register_uri_handler(server, &index_uri);

        // Route /status (Binds StatusHandler)
        httpd_uri_t status_uri = {
            .uri = "/status",
            .method = HTTP_GET,
            .handler = StatusHandler
        };
        httpd_register_uri_handler(server, &status_uri);

        // Route /favicon.ico
        register_favicon(server);

        ESP_LOGI(TAG, "HTTP Server Started with live status endpoint");
    }
}