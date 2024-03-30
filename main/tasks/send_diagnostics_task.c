#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "bm1397.h"
#include "mining.h"
#include "mbedtls/sha256.h"
#include "nvs_config.h"
#include "serial.h"
#include "system.h"
#include "global_state.h"
#include "esp_timer.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MAX_HTTP_OUTPUT_BUFFER 2048 // Adjust size according to the expected response
static const char *TAG = "InfluxDBSender";
static GlobalState * GLOBAL_STATE;

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Print the response for debugging (may be removed in production code)
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                printf("%.*s\n", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}


// The task to send data to InfluxDB
void influxdb_sender_task(void *pvParameters) {
    ESP_LOGI(TAG, "InfluxDB Sender Task started");
    GlobalState * GLOBAL_STATE = (GlobalState*)pvParameters;
    SystemModule * systemModule = &GLOBAL_STATE->SYSTEM_MODULE;
    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;
    char * username = nvs_config_get_string(NVS_CONFIG_STRATUM_USER, STRATUM_USER);
    GLOBAL_STATE->asic_model = nvs_config_get_string(NVS_CONFIG_ASIC_MODEL, "");

    static bool isUsernameHashed = false;
    static char hashedUsername[65] = {0}; // 64 hex chars + null terminator, initialized to all zeros


    if (!isUsernameHashed) {
        char *username = nvs_config_get_string(NVS_CONFIG_STRATUM_USER, STRATUM_USER);
        unsigned char hashOutput[32]; // SHA-256 outputs a 32-byte hash
        mbedtls_sha256((const unsigned char *)username, strlen(username), hashOutput, 0); // 0 for SHA-256
        
        for (int i = 0; i < 32; i++) {
            sprintf(&hashedUsername[i * 2], "%02x", hashOutput[i]);
        }

        isUsernameHashed = true;
    }    

    esp_http_client_config_t config = {
        .url = "http://datainflux.wantclue.de",
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    while (1) {
        double uptime_in_minutes = (double)esp_timer_get_time() / (60.0 * 1000000.0);
        int uptime_in_minutes_int = (int)uptime_in_minutes;
        ESP_LOGI(TAG, "Calculated uptime in minutes: %i", uptime_in_minutes_int);


        if (uptime_in_minutes_int > 0) {
        char data[1024];
        snprintf(data, sizeof(data),
                 "system_info,id=%s,model=%s hashRate=%.1f,Freq=%f,asicCurrent=%f,inputVoltage=%f,power=%f,temp=%f,uptime=%i",
                 hashedUsername,
                 GLOBAL_STATE->asic_model,
                 systemModule->current_hashrate,
                 power_management->frequency_value,
                 power_management->current,
                 power_management->voltage,
                 power_management->power,
                 power_management->chip_temp,
                 uptime_in_minutes_int);
        
        // Open the HTTP connection
        esp_http_client_open(client, strlen(data));

        // Write the POST data as binary
        esp_http_client_write(client, data, strlen(data));

        // Perform the HTTP POST request
        esp_err_t err = esp_http_client_fetch_headers(client);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP POST Request succeeded, status = %d", esp_http_client_get_status_code(client));
        } else {
            ESP_LOGE(TAG, "HTTP POST Request failed: %s", esp_err_to_name(err));
        }

        // Close the HTTP connection
        esp_http_client_close(client);
        } else {
            ESP_LOGI(TAG, "Uptime is less than 5 minutes; not sending data.");

        }
        // Wait for 30 seconds before the next post
        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    esp_http_client_cleanup(client);
}