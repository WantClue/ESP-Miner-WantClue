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
#include "display.h"
#include "display_interface.h"

#define SSD1306_I2C_ADDRESS    0x3C

#define LCD_H_RES              128
#define LCD_V_RES              32
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static const char * TAG = "ssd1306";

static esp_lcd_panel_handle_t panel_handle = NULL;
static bool display_state_on = false;

static lv_theme_t theme;
static lv_style_t scr_style;

extern const lv_font_t lv_font_portfolio_6x8;

static void theme_apply(lv_theme_t *theme, lv_obj_t *obj) {
    if (lv_obj_get_parent(obj) == NULL) {
        lv_obj_add_style(obj, &scr_style, LV_PART_MAIN);
    }
}

// SSD1306-specific implementation of display initialization
static esp_err_t ssd1306_init(void *pvParameters, lv_disp_t **disp_out) {
    GlobalState *GLOBAL_STATE = (GlobalState *)pvParameters;

    uint8_t flip_screen = nvs_config_get_u16(NVS_CONFIG_FLIP_SCREEN, 1);
    uint8_t invert_screen = nvs_config_get_u16(NVS_CONFIG_INVERT_SCREEN, 0);

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_RETURN_ON_ERROR(i2c_bitaxe_get_master_bus_handle(&i2c_master_bus_handle), TAG, "Failed to get i2c master bus handle");

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .scl_speed_hz = I2C_BUS_SPEED_HZ,
        .dev_addr = SSD1306_I2C_ADDRESS,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .dc_bit_offset = 6                     
    };
    
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_master_bus_handle, &io_config, &io_handle), TAG, "Failed to initialise i2c panel bus");

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &panel_handle), TAG, "No display found");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "Panel reset failed");
    esp_err_t esp_lcd_panel_init_err = esp_lcd_panel_init(panel_handle);
    if (esp_lcd_panel_init_err != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed, no display connected?");
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, invert_screen), TAG, "Panel invert failed");
        // ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, false, false), TAG, "Panel mirror failed");
    }
    
    ESP_LOGI(TAG, "Initialize LVGL");

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * LCD_V_RES,
        .double_buffer = true,
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .monochrome = true,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .rotation = {
            .swap_xy = false,
            .mirror_x = !flip_screen, // The screen is not flipped, this is for backwards compatibility
            .mirror_y = !flip_screen,
        },
        .flags = {
            .swap_bytes = false,
            .sw_rotate = false,
        }
    };

    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    if (disp_out) {
        *disp_out = disp;
    }

    if (esp_lcd_panel_init_err == ESP_OK) {
        if (lvgl_port_lock(0)) {
            lv_style_init(&scr_style);
            lv_style_set_text_font(&scr_style, &lv_font_portfolio_6x8);
            lv_style_set_bg_opa(&scr_style, LV_OPA_COVER);

            lv_theme_set_apply_cb(&theme, theme_apply);

            lv_display_set_theme(disp, &theme);
            lvgl_port_unlock();
        }

        // Only turn on the screen when it has been cleared
        esp_err_t esp_err = display_on(true);
        if (ESP_OK != esp_err) {
            return esp_err;
        }

        GLOBAL_STATE->SYSTEM_MODULE.is_screen_active = true;
    } else {
        ESP_LOGW(TAG, "No display found.");
    }

    return ESP_OK;
}

// Standard screen creation functions for SSD1306
static lv_obj_t *ssd1306_create_scr_self_test(void) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "BITAXE SELF TEST");

    lv_obj_t *message_label = lv_label_create(scr);
    lv_obj_t *result_label = lv_label_create(scr);

    lv_obj_t *finished_label = lv_label_create(scr);
    lv_obj_set_width(finished_label, LV_HOR_RES);
    lv_label_set_long_mode(finished_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    return scr;
}

static lv_obj_t *ssd1306_create_scr_overheat(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "DEVICE OVERHEAT!");

    lv_obj_t *label2 = lv_label_create(scr);
    lv_obj_set_width(label2, LV_HOR_RES);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(label2, "Power, frequency and fan configurations have been reset. Go to AxeOS to reconfigure device.");

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "IP Address:");

    lv_obj_t *ip_label = lv_label_create(scr);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_asic_status(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label = lv_label_create(scr);
    lv_obj_set_width(label, LV_HOR_RES);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_configure(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "CONFIGURE WIFI");

    lv_obj_t *label2 = lv_label_create(scr);
    lv_obj_set_width(label2, LV_HOR_RES);
    lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(label2, "Connect to Bitaxe WiFi AP and go to 192.168.4.1 to configure.");
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_ota(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "FIRMWARE UPDATE");

    lv_obj_t *label2 = lv_label_create(scr);
    lv_label_set_text(label2, "File:");

    lv_obj_t *filename_label = lv_label_create(scr);

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "Status:");

    lv_obj_t *status_label = lv_label_create(scr);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_connection(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "WIFI STATUS");

    lv_obj_t *status_label = lv_label_create(scr);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_logo(const lv_img_dsc_t *img_dsc) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, img_dsc);
    lv_obj_center(img);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_urls(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *label1 = lv_label_create(scr);
    lv_label_set_text(label1, "Stratum Host:");

    lv_obj_t *mining_url_label = lv_label_create(scr);
    lv_obj_set_width(mining_url_label, LV_HOR_RES);
    lv_label_set_long_mode(mining_url_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_obj_t *label3 = lv_label_create(scr);
    lv_label_set_text(label3, "IP Address:");

    lv_obj_t *ip_addr_label = lv_label_create(scr);
    
    return scr;
}

static lv_obj_t *ssd1306_create_scr_stats(void) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *hashrate_label = lv_label_create(scr);
    lv_label_set_text(hashrate_label, "Gh/s: --");

    lv_obj_t *efficiency_label = lv_label_create(scr);
    lv_label_set_text(efficiency_label, "J/Th: --");

    lv_obj_t *difficulty_label = lv_label_create(scr);
    lv_label_set_text(difficulty_label, "Best: --");

    lv_obj_t *chip_temp_label = lv_label_create(scr);
    lv_label_set_text(chip_temp_label, "Temp: --");
    
    return scr;
}

// SSD1306-specific implementation of display_on
static esp_err_t ssd1306_display_on(bool on)
{
    if (NULL != panel_handle) {
        if (on && !display_state_on) {
            ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "Panel display on failed");
            display_state_on = true;
        }
        else if (!on && display_state_on)
        {
            ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, false), TAG, "Panel display off failed");
            display_state_on = false;
        }
    }

    return ESP_OK;
}

// SSD1306 display interface
static const display_interface_t ssd1306_interface = {
    .width = LCD_H_RES,
    .height = LCD_V_RES,
    .init = ssd1306_init,
    .display_on = ssd1306_display_on,
    .create_scr_self_test = ssd1306_create_scr_self_test,
    .create_scr_overheat = ssd1306_create_scr_overheat,
    .create_scr_asic_status = ssd1306_create_scr_asic_status,
    .create_scr_configure = ssd1306_create_scr_configure,
    .create_scr_ota = ssd1306_create_scr_ota,
    .create_scr_connection = ssd1306_create_scr_connection,
    .create_scr_logo = ssd1306_create_scr_logo,
    .create_scr_urls = ssd1306_create_scr_urls,
    .create_scr_stats = ssd1306_create_scr_stats,
    .anim_duration = LV_DEF_REFR_PERIOD * 128 / 8,
    .scroll_speed = 10
};

// Export the SSD1306 interface
const display_interface_t *ssd1306_get_interface(void) {
    return &ssd1306_interface;
}