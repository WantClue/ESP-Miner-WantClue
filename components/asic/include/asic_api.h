// asic_api.h
#ifndef ASIC_API_H
#define ASIC_API_H

#include <stdint.h>
#include <stdbool.h>
#include "asic_types.h"
#include "mining.h"
#include "common.h"

typedef struct {
    void (*set_version_mask)(uint32_t version_mask);
    bool (*set_frequency)(float target_freq);
    void (*set_job_difficulty_mask)(int difficulty);
    uint8_t (*init)(uint64_t frequency, uint16_t asic_count);
    int (*set_default_baud)(void);
    int (*set_max_baud)(void);
    void (*send_work)(void* pvParameters, bm_job* next_bm_job);
    task_result* (*process_work)(void* pvParameters);
} asic_ops_t;

// Initialize the ASIC API with specific chip type
bool asic_api_init(asic_type_t chip_type);

// Common interface functions
void asic_do_frequency_ramp_up(float target_frequency);
bool asic_set_frequency(float target_freq);
void asic_set_version_mask(uint32_t version_mask);
void asic_set_job_difficulty_mask(int difficulty);
uint8_t asic_init(uint64_t frequency, uint16_t asic_count);
int asic_set_default_baud(void);
int asic_set_max_baud(void);
void asic_send_work(void* pvParameters, bm_job* next_bm_job);
task_result* asic_process_work(void* pvParameters);

// Common utility functions that can be shared across different chips
bool asic_do_frequency_transition(float current_freq, float target_freq, bool (*set_freq_fn)(float));

#endif // ASIC_API_H