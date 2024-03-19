#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "global_state.h"


void influxdb_sender_task(void *pvParameter) {
    ESP_LOGI("InfluxDBSender", "Task started");

    // Configure HTTP client for InfluxDB
    esp_http_client_config_t config = {
        .url = "http://10.0.10.7.net:8086/api/v2/write?org=nther&bucket=bitaxe&precision=s",
        .method = HTTP_METHOD_POST,
        .headers = "Authorization: Token VvA66ZLtvF-RcytZivMPqG7RThi2RZ_itxleXKsiw0wfxM3e3bTgBbrbi6mmUPFz84G8lNTcdCO3nHME1RSS5w==",
    };

    while (1) {
        // Collect data
        char data[512];
        snprintf(data, sizeof(data),
                 "system_info board_version=\"%s\",temp=%.2f,hashRate=%llu,coreVoltage=%.2f,coreVoltageActual=%.2f,frequency=%u,version=\"%s\"",
                 GLOBAL_STATE->board_version,
                 GLOBAL_STATE->POWER_MANAGEMENT_MODULE.chip_temp,
                 GLOBAL_STATE->SYSTEM_MODULE.current_hashrate,
                 GLOBAL_STATE->POWER_MANAGEMENT_MODULE.voltage,
                 GLOBAL_STATE->SYSTEM_MODULE.current
                 ADC_get_vcore(),  
                 GLOBAL_STATE->ASIC_FREQ, 
                 esp_app_get_description()->version  // Assuming this gets the firmware version
                 );

        // Send data
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_post_field(client, data, strlen(data));
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI("InfluxDBSender", "Data sent successfully");
        } else {
            ESP_LOGE("InfluxDBSender", "Failed to send data: %s", esp_err_to_name(err));
        }
        esp_http_client_cleanup(client);

        vTaskDelay(pdMS_TO_TICKS(300000));  // 300 seconds delay
    }
}
