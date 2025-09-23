#ifndef HASHRATE_MONITOR_TASK_H_
#define HASHRATE_MONITOR_TASK_H_

#include <stdint.h>
#include <stdbool.h>

#define REG_NONCE_TOTAL_CNT 0x8C

typedef struct {
    uint32_t period_ms;
    uint32_t window_ms;
    uint32_t settle_ms;

    uint64_t window_us;

    void *global_state;
} hashrate_monitor_t;

// Start the background task. period_ms = cadence of measurements,
// window_ms = measurement window length before READ, settle_ms = RX settle time after READ.
bool hashrate_monitor_start(hashrate_monitor_t *monitor, void *global_state, uint32_t period_ms, uint32_t window_ms, uint32_t settle_ms);

// Called from RX dispatcher for each register reply.
// 'data_ticks' is the 32-bit counter (host-endian).
void hashrate_monitor_on_register_reply(hashrate_monitor_t *monitor, uint8_t asic_idx, uint32_t data_ticks);

#endif /* HASHRATE_MONITOR_TASK_H_ */