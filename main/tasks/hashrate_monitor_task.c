#include "hashrate_monitor_task.h"
#include "bm1370.h"
#include "global_state.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *HR_TAG = "hashrate_monitor";
static const double ERRATA_FACTOR = 1.046; // default +4.6%

static void hashrate_monitor_task_loop(void *pv)
{
    hashrate_monitor_t *self = (hashrate_monitor_t *)pv;

    // Small startup delay
    vTaskDelay(pdMS_TO_TICKS(3000));

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(self->period_ms));

        if (!self->global_state)
            continue;

        // Precise start timestamp (µs) taken right before RESET.
        uint64_t m_t0_us = esp_timer_get_time();

        // Send broadcast RESET for 0x8C.
        BM1370_reset_counter(REG_NONCE_TOTAL_CNT);

        // Measurement window (blocking here
        vTaskDelay(pdMS_TO_TICKS(self->window_ms));

        // Measure actual window length in µs and then READ back all counters
        self->window_us = esp_timer_get_time() - m_t0_us;
        BM1370_read_counter(REG_NONCE_TOTAL_CNT);

        // Give RX some time to deliver replies
        vTaskDelay(pdMS_TO_TICKS(self->settle_ms));

        // In case not all replies arrived, still publish what we have
        // publishTotalIfComplete(self);
    }
}

static void publish_total_if_complete(hashrate_monitor_t *monitor)
{
    // Assuming board has getTotalChipHashrate function
    // float hashrate = board_get_total_chip_hashrate(monitor->board);
    // ESP_LOGI(HR_TAG, "total hash reported by chips: %.3f GH/s", hashrate);
}

bool hashrate_monitor_start(hashrate_monitor_t *monitor, void *global_state, uint32_t period_ms, uint32_t window_ms, uint32_t settle_ms)
{
    monitor->global_state = global_state;
    monitor->period_ms = period_ms;
    monitor->window_ms = window_ms;
    monitor->settle_ms = settle_ms;

    if (!monitor->global_state) {
        ESP_LOGE(HR_TAG, "start(): missing global_state");
        return false;
    }

    xTaskCreate(&hashrate_monitor_task_loop, "hr_monitor", 4096, (void *)monitor, 10, NULL);
    ESP_LOGI(HR_TAG, "started (period=%lums, window=%lums, settle=%lums)", monitor->period_ms, monitor->window_ms, monitor->settle_ms);
    return true;
}

void hashrate_monitor_on_register_reply(hashrate_monitor_t *monitor, uint8_t asic_idx, uint32_t data_ticks)
{
    // no valid window yet?
    if (monitor->window_us == 0) {
        return;
    }

    // GH/s = cnt * 4096 / (window_s) / 1e9  ==> GH/s = cnt * 4.096 / window_us
    double chip_ghs = (double) data_ticks * 4.096e6 * ERRATA_FACTOR / (double) monitor->window_us;

    if (monitor->global_state && asic_idx < 16) {
        GlobalState *gs = (GlobalState *)monitor->global_state;
        gs->SYSTEM_MODULE.chip_hashrates[asic_idx] = chip_ghs;
        ESP_LOGI(HR_TAG, "hashrate of chip %u: %.3f GH/s", asic_idx, (float) chip_ghs);
    }
}