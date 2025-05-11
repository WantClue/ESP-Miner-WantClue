#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "lvgl.h"
#include "lvgl__lvgl/src/themes/lv_theme_private.h"
#include "esp_lvgl_port.h"
#include "global_state.h"
#include "nvs_config.h"
#include "i2c_bitaxe.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_lcd_panel_ssd1306.h"
#include "display_interface.h"

static const char * TAG = "display";

static esp_lcd_panel_handle_t panel_handle = NULL;
static bool display_state_on = false;
static const display_interface_t *current_display_interface = NULL;

// Forward declarations for display interfaces
extern const display_interface_t *ssd1306_get_interface(void);
extern const display_interface_t *ssd1309_get_interface(void);

// Get the appropriate display interface based on display type
const display_interface_t *display_get_interface(uint8_t display_type) {
    switch (display_type) {
        case DISPLAY_TYPE_SSD1309:
            return ssd1309_get_interface();
        case DISPLAY_TYPE_SSD1306:
        default:
            return ssd1306_get_interface();
    }
}

esp_err_t display_on(bool display_on)
{
    if (current_display_interface != NULL) {
        return current_display_interface->display_on(display_on);
    }
    return ESP_OK;
}

esp_err_t display_init(void * pvParameters)
{
    GlobalState * GLOBAL_STATE = (GlobalState *) pvParameters;

    // Get display type from NVS
    uint8_t display_type = nvs_config_get_u16(NVS_CONFIG_DISPLAY_TYPE, DISPLAY_TYPE_SSD1306);
    
    // Get the appropriate display interface
    current_display_interface = display_get_interface(display_type);
    
    ESP_LOGI(TAG, "Initializing display type: %s (%dx%d)", 
             (display_type == DISPLAY_TYPE_SSD1309) ? "SSD1309" : "SSD1306", 
             current_display_interface->width, current_display_interface->height);
    
    // Initialize the display using the interface
    lv_disp_t *disp = NULL;
    return current_display_interface->init(pvParameters, &disp);
}

// Getter for the current display interface
const display_interface_t *get_current_display_interface(void) {
    return current_display_interface;
}
