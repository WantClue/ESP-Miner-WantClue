#ifndef POWER_MANAGEMENT_TASK_H_
#define POWER_MANAGEMENT_TASK_H_

#include "asic_api.h"

typedef struct
{
    uint16_t fan_perc;
    uint16_t fan_rpm;
    float chip_temp[6];
    float chip_temp_avg;
    float vr_temp;
    float voltage;
    float frequency_multiplier;
    float frequency_value;
    float power;
    float current;
    bool HAS_POWER_EN;
    bool HAS_PLUG_SENSE;
    asic_type_t asic_type;
} PowerManagementModule;

bool power_management_init(PowerManagementModule* module, asic_type_t chip_type);
void POWER_MANAGEMENT_task(void * pvParameters);

#endif
