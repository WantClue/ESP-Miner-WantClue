#include <math.h>
#include "asic_api.h"
#include "bm1366.h"
#include "bm1368.h"
#include "bm1370.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "asic_api";
static asic_ops_t current_asic_ops;
static asic_type_t current_asic_type;

static const asic_ops_t bm1366_ops = {
    .set_version_mask = BM1366_set_version_mask,
    .set_frequency = BM1366_send_hash_frequency,
    .set_job_difficulty_mask = BM1366_set_job_difficulty_mask,
    .init = BM1366_init,
    .set_default_baud = BM1366_set_default_baud,
    .set_max_baud = BM1366_set_max_baud,
    .send_work = BM1366_send_work,
    .process_work = BM1366_proccess_work
};

static const asic_ops_t bm1368_ops = {
    .set_version_mask = BM1368_set_version_mask,
    .set_frequency = BM1368_send_hash_frequency,
    .set_job_difficulty_mask = BM1368_set_job_difficulty_mask,
    .init = BM1368_init,
    .set_default_baud = BM1368_set_default_baud,
    .set_max_baud = BM1368_set_max_baud,
    .send_work = BM1368_send_work,
    .process_work = BM1368_proccess_work
};

static const asic_ops_t bm1370_ops = {
    .set_version_mask = BM1370_set_version_mask,
    .set_frequency = BM1370_send_hash_frequency,
    .set_job_difficulty_mask = BM1370_set_job_difficulty_mask,
    .init = BM1370_init,
    .set_default_baud = BM1370_set_default_baud,
    .set_max_baud = BM1370_set_max_baud,
    .send_work = BM1370_send_work,
    .process_work = BM1370_proccess_work
};

bool asic_api_init(asic_type_t chip_type) {
    current_asic_type = chip_type;
    
    switch (chip_type) {
        case ASIC_BM1366:
            current_asic_ops = bm1366_ops;
            break;
        case ASIC_BM1368:
            current_asic_ops = bm1368_ops;
            break;
        case ASIC_BM1370:
            current_asic_ops = bm1370_ops;
            break;
        default:
            ESP_LOGE(TAG, "Unsupported ASIC type");
            return false;
    }
    
    return true;
}

// Common interface implementations
bool asic_set_frequency(float target_freq) {
    return current_asic_ops.set_frequency(target_freq);
}

void asic_set_version_mask(uint32_t version_mask) {
    current_asic_ops.set_version_mask(version_mask);
}

void asic_set_job_difficulty_mask(int difficulty) {
    current_asic_ops.set_job_difficulty_mask(difficulty);
}

uint8_t asic_init(uint64_t frequency, uint16_t asic_count) {
    return current_asic_ops.init(frequency, asic_count);
}

int asic_set_default_baud(void) {
    return current_asic_ops.set_default_baud();
}

int asic_set_max_baud(void) {
    return current_asic_ops.set_max_baud();
}

void asic_send_work(void* pvParameters, bm_job* next_bm_job) {
    current_asic_ops.send_work(pvParameters, next_bm_job);
}

task_result* asic_process_work(void* pvParameters) {
    return current_asic_ops.process_work(pvParameters);
}

void asic_do_frequency_ramp_up(float target_frequency) {
    float current_freq = 56.25;  // Starting frequency for all chips
    
    ESP_LOGI(TAG, "Ramping up frequency from %.2f MHz to %.2f MHz", current_freq, target_frequency);
    
    asic_do_frequency_transition(current_freq, target_frequency, current_asic_ops.set_frequency);
}

// Shared utility implementation
bool asic_do_frequency_transition(float current_freq, float target_freq, bool (*set_freq_fn)(float)) {
    float step = 6.25;
    float current = current_freq;
    float target = target_freq;
    float direction = (target > current) ? step : -step;

    ESP_LOGI(TAG, "Starting frequency transition from %.2f MHz to %.2f MHz in %.2f MHz steps", 
             current, target, step);

    // Handle non-aligned current frequency
    if (fmod(current, step) != 0) {
        float next_dividable = (direction > 0) ? 
            ceil(current / step) * step : 
            floor(current / step) * step;
        current = next_dividable;
        ESP_LOGI(TAG, "Adjusting to aligned frequency: %.2f MHz", current);
        if (!set_freq_fn(current)) {
            ESP_LOGE(TAG, "Failed to set aligned frequency");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Perform frequency transition
    while ((direction > 0 && current < target) || 
           (direction < 0 && current > target)) {
        float next_step = fmin(fabs(direction), fabs(target - current));
        current += direction > 0 ? next_step : -next_step;
        ESP_LOGI(TAG, "Transitioning to: %.2f MHz", current);
        if (!set_freq_fn(current)) {
            ESP_LOGE(TAG, "Failed to set frequency %.2f MHz", current);
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Final frequency setting
    if (current != target) {
        ESP_LOGI(TAG, "Setting final frequency: %.2f MHz", target);
        return set_freq_fn(target);
    }

    ESP_LOGI(TAG, "Frequency transition complete");
    return true;
}