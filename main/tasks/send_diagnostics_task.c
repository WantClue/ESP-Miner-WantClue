#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "bm1397.h"
#include "mining.h"
#include "nvs_config.h"
#include "serial.h"
#include "DS4432U.h"
#include "EMC2101.h"
#include "INA260.h"
#include "global_state.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void influxdb_sender_task(void *pvParameter) {
    ESP_LOGI("InfluxDBSender", "Task started");

    // Configure HTTP client for InfluxDB
    esp_http_client_config_t config = {
        .url = "http://10.0.10.7:8086/api/v2/write?org=nther&bucket=bitaxe&precision=s",
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Authorization", "Token VvA66ZLtvF-RcytZivMPqG7RThi2RZ_itxleXKsiw0wfxM3e3bTgBbrbi6mmUPFz84G8lNTcdCO3nHME1RSS5w==");
    
    GlobalState * GLOBAL_STATE = (GlobalState *) pvParameters;


    while (1) {
        // Collect data
        char data[512];
        snprintf(data, sizeof(data),
                 "system_info board_version=\"%s\",temp=%.2f,hashRate=%llu,coreVoltage=%.2f,coreVoltageActual=%.2f,frequency=%u,version=\"%s\"",
                 //GLOBAL_STATE->board_version,
                 GLOBAL_STATE->POWER_MANAGEMENT_MODULE.chip_temp,
                 GLOBAL_STATE->SYSTEM_MODULE.current_hashrate,
                 GLOBAL_STATE->POWER_MANAGEMENT_MODULE.voltage,
                 //ADC_get_vcore(),  // Assuming this gets the actual core voltage
                 GLOBAL_STATE->ASIC_FREQ,  // Assuming this is how you get the ASIC frequency
                 //esp_app_get_description()->version  // Assuming this gets the firmware version
        );

        esp_http_client_set_post_field(client, data, strlen(data));
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI("InfluxDBSender", "Data sent successfully");
        } else {
            ESP_LOGE("InfluxDBSender", "Failed to send data: %s", esp_err_to_name(err));
        }
        esp_http_client_cleanup(client);

        vTaskDelay(pdMS_TO_TICKS(30000));  // 30 seconds delay
    }
}
