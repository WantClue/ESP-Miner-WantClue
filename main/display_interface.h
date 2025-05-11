#ifndef MAIN_DISPLAY_INTERFACE_H
#define MAIN_DISPLAY_INTERFACE_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "lvgl.h"
#include "global_state.h"

// Display types
#define DISPLAY_TYPE_SSD1306   0
#define DISPLAY_TYPE_SSD1309   1

// Common display interface
typedef struct {
    // Display properties
    uint16_t width;
    uint16_t height;
    
    // Initialization function
    esp_err_t (*init)(void *pvParameters, lv_disp_t **disp_out);
    
    // Display on/off function
    esp_err_t (*display_on)(bool on);
    
    // Screen creation functions
    lv_obj_t *(*create_scr_self_test)(void);
    lv_obj_t *(*create_scr_overheat)(SystemModule *module);
    lv_obj_t *(*create_scr_asic_status)(SystemModule *module);
    lv_obj_t *(*create_scr_configure)(SystemModule *module);
    lv_obj_t *(*create_scr_ota)(SystemModule *module);
    lv_obj_t *(*create_scr_connection)(SystemModule *module);
    lv_obj_t *(*create_scr_logo)(const lv_img_dsc_t *img_dsc);
    lv_obj_t *(*create_scr_urls)(SystemModule *module);
    lv_obj_t *(*create_scr_stats)(void);
    lv_obj_t *(*create_scr_wifi)(SystemModule *module);
    
    // Animation settings
    uint32_t anim_duration;
    uint32_t scroll_speed;
} display_interface_t;

// Get the appropriate display interface based on display type
const display_interface_t *display_get_interface(uint8_t display_type);

#endif // MAIN_DISPLAY_INTERFACE_H