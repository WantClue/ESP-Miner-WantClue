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

#define SSD1309_I2C_ADDRESS    0x3C

#define LCD_H_RES              128
#define LCD_V_RES              64  // SSD1309 has 64 pixels in height
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static const char * TAG = "ssd1309";

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

// SSD1309-specific implementation of display initialization
static esp_err_t ssd1309_init(void *pvParameters, lv_disp_t **disp_out) {
    GlobalState *GLOBAL_STATE = (GlobalState *)pvParameters;

    uint8_t flip_screen = nvs_config_get_u16(NVS_CONFIG_FLIP_SCREEN, 1);
    uint8_t invert_screen = nvs_config_get_u16(NVS_CONFIG_INVERT_SCREEN, 0);

    i2c_master_bus_handle_t i2c_master_bus_handle;
    ESP_RETURN_ON_ERROR(i2c_bitaxe_get_master_bus_handle(&i2c_master_bus_handle), TAG, "Failed to get i2c master bus handle");

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .scl_speed_hz = I2C_BUS_SPEED_HZ,
        .dev_addr = SSD1309_I2C_ADDRESS,
        .control_phase_bytes = 1,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .dc_bit_offset = 6                     
    };
    
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(i2c_master_bus_handle, &io_config, &io_handle), TAG, "Failed to initialise i2c panel bus");

    ESP_LOGI(TAG, "Install SSD1309 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .bits_per_pixel = 1,
        .reset_gpio_num = -1,
    };

    // SSD1309 has 64 pixels in height
    esp_lcd_panel_ssd1306_config_t ssd1309_config = {
        .height = LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1309_config;

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
            .mirror_x = !flip_screen,
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

// Optimized screen creation functions for SSD1309
static lv_obj_t *ssd1309_create_scr_self_test(void) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "BITAXE SELF TEST");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Create a container
    lv_obj_t *container = lv_obj_create(scr);
    lv_obj_set_size(container, LCD_H_RES, LCD_V_RES - 20);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(container, 2, 0);

    lv_obj_t *message_label = lv_label_create(container);
    lv_obj_t *result_label = lv_label_create(container);

    lv_obj_t *finished_label = lv_label_create(container);
    lv_obj_set_width(finished_label, LCD_H_RES - 4);
    lv_label_set_long_mode(finished_label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    return scr;
}

static lv_obj_t *ssd1309_create_scr_overheat(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title with larger font
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "DEVICE OVERHEAT!");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Warning message
    lv_obj_t *warning = lv_label_create(scr);
    lv_obj_set_width(warning, LCD_H_RES);
    lv_label_set_long_mode(warning, LV_LABEL_LONG_WRAP);
    lv_label_set_text(warning, "Power, frequency and fan configurations have been reset. Go to AxeOS to reconfigure device.");

    // IP Address section
    lv_obj_t *ip_label = lv_label_create(scr);
    lv_label_set_text(ip_label, "IP Address:");

    lv_obj_t *ip_value = lv_label_create(scr);
    
    lv_obj_t *chip_temp_label = lv_label_create(scr);
    lv_label_set_text(chip_temp_label, "Temp: --");

    return scr;
}

static lv_obj_t *ssd1309_create_scr_asic_status(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "ASIC STATUS");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Status message with scrolling
    lv_obj_t *status = lv_label_create(scr);
    lv_obj_set_width(status, LCD_H_RES);
    lv_label_set_long_mode(status, LV_LABEL_LONG_SCROLL_CIRCULAR);
    
    // Additional status information (utilizing the larger display)
    lv_obj_t *hashrate = lv_label_create(scr);
    lv_label_set_text_fmt(hashrate, "Hashrate: %.2f GH/s", module->current_hashrate);
    
    lv_obj_t *chip_temp_label = lv_label_create(scr);
    lv_label_set_text(chip_temp_label, "Temp: --");
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_configure(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "CONFIGURE WIFI");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Instructions
    lv_obj_t *instructions = lv_label_create(scr);
    lv_obj_set_width(instructions, LCD_H_RES);
    lv_label_set_long_mode(instructions, LV_LABEL_LONG_WRAP);
    lv_label_set_text(instructions, "Connect to Bitaxe WiFi AP and go to 192.168.4.1 to configure.");
    
    // QR code placeholder (could be implemented with actual QR code)
    lv_obj_t *qr_label = lv_label_create(scr);
    lv_label_set_text(qr_label, "Scan QR code with phone:");
    
    lv_obj_t *qr_placeholder = lv_obj_create(scr);
    lv_obj_set_size(qr_placeholder, 40, 40);
    lv_obj_set_style_border_width(qr_placeholder, 1, 0);
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_ota(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "FIRMWARE UPDATE");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // File info
    lv_obj_t *file_label = lv_label_create(scr);
    lv_label_set_text(file_label, "File:");

    lv_obj_t *filename = lv_label_create(scr);
    lv_obj_set_width(filename, LCD_H_RES);
    lv_label_set_long_mode(filename, LV_LABEL_LONG_SCROLL_CIRCULAR);

    // Status info
    lv_obj_t *status_label = lv_label_create(scr);
    lv_label_set_text(status_label, "Status:");

    lv_obj_t *status = lv_label_create(scr);
    
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_connection(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WIFI STATUS");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Status
    lv_obj_t *status = lv_label_create(scr);
    
    // Additional connection details
    lv_obj_t *ssid_label = lv_label_create(scr);
    lv_label_set_text_fmt(ssid_label, "SSID: %s", module->ssid);
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_logo(const lv_img_dsc_t *img_dsc) {
    lv_obj_t *scr = lv_obj_create(NULL);

    // Center the logo
    lv_obj_t *img = lv_img_create(scr);
    lv_img_set_src(img, img_dsc);
    lv_obj_center(img);
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_urls(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Mining pool info
    lv_obj_t *pool_label = lv_label_create(scr);
    lv_label_set_text(pool_label, "Stratum Host:");

    lv_obj_t *mining_url = lv_label_create(scr);
    lv_obj_set_width(mining_url, LCD_H_RES);
    lv_label_set_long_mode(mining_url, LV_LABEL_LONG_SCROLL_CIRCULAR);
    
    // Additional connection details
    lv_obj_t *user_label = lv_label_create(scr);
    lv_label_set_text(user_label, "User:");
    
    lv_obj_t *user = lv_label_create(scr);
    lv_obj_set_width(user, LCD_H_RES);
    lv_label_set_long_mode(user, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(user, module->is_using_fallback ? module->fallback_pool_user : module->pool_user);
    
    return scr;
}

static lv_obj_t *ssd1309_create_scr_stats(void) {
    lv_obj_t *scr = lv_obj_create(NULL);

    // Set up the main screen with column layout
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "MINING STATISTICS");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);
    
    // Create a container for the stats (expected by screen.c)
    lv_obj_t *stats_container = lv_obj_create(scr);
    lv_obj_set_size(stats_container, LCD_H_RES, LCD_V_RES - 15); // Adjusted height
    lv_obj_set_flex_flow(stats_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(stats_container, 0, 0);
    lv_obj_set_style_pad_row(stats_container, 0, 0); // No space between rows

    // Row 1: Contains hashrate and efficiency (expected by screen.c)
    lv_obj_t *row1 = lv_obj_create(stats_container);
    lv_obj_set_size(row1, LCD_H_RES - 4, 18); // Reduced height
    lv_obj_set_flex_flow(row1, LV_FLEX_FLOW_COLUMN); // Column layout so labels appear on separate lines
    lv_obj_set_style_pad_all(row1, 0, 0);
    lv_obj_set_style_pad_row(row1, 0, 0); // No space between rows
    
    // Hashrate label (first child of row1)
    lv_obj_t *hashrate_label = lv_label_create(row1);
    lv_label_set_text(hashrate_label, "Gh/s: --");
    
    // Efficiency label (second child of row1)
    lv_obj_t *efficiency_label = lv_label_create(row1);
    lv_label_set_text(efficiency_label, "J/Th: --");
    
    // Difficulty label (second child of stats_container)
    lv_obj_t *difficulty_label = lv_label_create(stats_container);
    lv_label_set_text(difficulty_label, "Best: --");
    lv_label_set_long_mode(difficulty_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_margin_top(difficulty_label, 0, 0); // No top margin
    lv_obj_set_style_margin_bottom(difficulty_label, 0, 0); // No bottom margin
    
    // Row 3: Contains temperature (third child of stats_container)
    lv_obj_t *row3 = lv_obj_create(stats_container);
    lv_obj_set_size(row3, LCD_H_RES - 4, 10); // Reduced height
    lv_obj_set_style_pad_all(row3, 0, 0);
    lv_obj_set_style_margin_top(row3, 0, 0); // No top margin
    lv_obj_set_style_margin_bottom(row3, 0, 0); // No bottom margin
    
    // Temperature label (first child of row3)
    lv_obj_t *chip_temp_label = lv_label_create(row3);
    lv_label_set_text(chip_temp_label, "Temp: --");
    
    // Row 4: Contains shares and rejected (fourth child of stats_container)
    lv_obj_t *row4 = lv_obj_create(stats_container);
    lv_obj_set_size(row4, LCD_H_RES - 4, 10); // Reduced height
    lv_obj_set_flex_flow(row4, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(row4, 0, 0);
    lv_obj_set_style_margin_top(row4, 0, 0); // No top margin
    
    // Shares label (first child of row4)
    lv_obj_t *shares_label = lv_label_create(row4);
    lv_label_set_text(shares_label, "Shares: --");
    
    // Rejected label (second child of row4) - with 2-column layout
    lv_obj_t *rejected_label = lv_label_create(row4);
    lv_label_set_text(rejected_label, "Rej: --");
    lv_obj_set_flex_grow(rejected_label, 1);
    lv_obj_set_style_text_align(rejected_label, LV_TEXT_ALIGN_RIGHT, 0);
    
    return scr;
}

// WIFI Screen
static lv_obj_t *ssd1309_create_scr_wifi(SystemModule *module) {
    lv_obj_t *scr = lv_obj_create(NULL);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WIFI STATISTICS");
    lv_obj_set_style_text_font(title, &lv_font_portfolio_6x8, 0);

    // Set up the main screen with column layout
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // IP address
    lv_obj_t *ip_label = lv_label_create(scr);
    lv_label_set_text(ip_label, "IP Address:");
    
    lv_obj_t *ip_addr = lv_label_create(scr);

    // Additional connection details
    lv_obj_t *ssid_label = lv_label_create(scr);
    lv_label_set_text_fmt(ssid_label, "SSID: %s", module->ssid);
    
    lv_obj_t *rssi_label = lv_label_create(scr);
    lv_label_set_text_fmt(rssi_label, "Signal: %d dBm", module->wifi_rssi);

    return scr;
}

// SSD1309-specific implementation of display_on
static esp_err_t ssd1309_display_on(bool on)
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

// SSD1309 display interface with optimized animation settings
static const display_interface_t ssd1309_interface = {
    .width = LCD_H_RES,
    .height = LCD_V_RES,
    .init = ssd1309_init,
    .create_scr_self_test = ssd1309_create_scr_self_test,
    .create_scr_overheat = ssd1309_create_scr_overheat,
    .create_scr_asic_status = ssd1309_create_scr_asic_status,
    .create_scr_configure = ssd1309_create_scr_configure,
    .create_scr_ota = ssd1309_create_scr_ota,
    .create_scr_connection = ssd1309_create_scr_connection,
    .create_scr_logo = ssd1309_create_scr_logo,
    .create_scr_urls = ssd1309_create_scr_urls,
    .create_scr_stats = ssd1309_create_scr_stats,
    .create_scr_wifi = ssd1309_create_scr_wifi,
    .anim_duration = LV_DEF_REFR_PERIOD * 128 / 16, // Faster animation for SSD1309
    .display_on = ssd1309_display_on,
};

// Export the SSD1309 interface
const display_interface_t *ssd1309_get_interface(void) {
    return &ssd1309_interface;
}