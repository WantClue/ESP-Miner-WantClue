#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_check.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "global_state.h"
#include "screen.h"
#include "nvs_config.h"
#include "display.h"
#include "display_interface.h"

// Forward declarations for display interfaces
extern const display_interface_t *ssd1306_get_interface(void);
extern const display_interface_t *ssd1309_get_interface(void);

typedef enum {
    SCR_SELF_TEST,
    SCR_OVERHEAT,
    SCR_ASIC_STATUS,
    SCR_CONFIGURE,
    SCR_FIRMWARE_UPDATE,
    SCR_CONNECTION,
    SCR_BITAXE_LOGO,
    SCR_OSMU_LOGO,
    SCR_URLS,
    SCR_STATS,
    SCR_WIFI,
    MAX_SCREENS,
} screen_t;

#define SCREEN_UPDATE_MS 500

#define SCR_CAROUSEL_START SCR_URLS
#define SCR_CAROUSEL_END SCR_WIFI

extern const lv_img_dsc_t bitaxe_logo;
extern const lv_img_dsc_t osmu_logo;

static lv_obj_t * screens[MAX_SCREENS];
static int delays_ms[MAX_SCREENS] = {0, 0, 0, 0, 0, 1000, 3000, 3000, 10000, 10000, 10000};

static screen_t current_screen = -1;
static int current_screen_time_ms;
static int current_screen_delay_ms;

static GlobalState * GLOBAL_STATE;

static lv_obj_t *asic_status_label;

static lv_obj_t *hashrate_label;
static lv_obj_t *efficiency_label;
static lv_obj_t *difficulty_label;
static lv_obj_t *chip_temp_label;
static lv_obj_t *shares_label;
static lv_obj_t *rejected_label;

static lv_obj_t *firmware_update_scr_filename_label;
static lv_obj_t *firmware_update_scr_status_label;
static lv_obj_t *ip_addr_scr_overheat_label;
static lv_obj_t *ip_addr_scr_urls_label;
static lv_obj_t *ip_addr_scr_wifi_label;
static lv_obj_t *mining_url_scr_urls_label;
static lv_obj_t *wifi_status_label;
static lv_obj_t *wifi_rssi_label;

static lv_obj_t *self_test_message_label;
static lv_obj_t *self_test_result_label;
static lv_obj_t *self_test_finished_label;

static double current_hashrate;
static float current_power;
static uint64_t current_difficulty;
static float current_chip_temp;
static bool found_block;
static lv_obj_t * create_scr_self_test() {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_self_test();
    
    // Store references to the labels we need to update
    self_test_message_label = lv_obj_get_child(scr, 1);
    self_test_result_label = lv_obj_get_child(scr, 2);
    self_test_finished_label = lv_obj_get_child(scr, 3);
    
    return scr;
}

static lv_obj_t * create_scr_overheat(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_overheat(module);
    
    // Store reference to the IP address label
    ip_addr_scr_overheat_label = lv_obj_get_child(scr, 3);
    
    return scr;
}

static lv_obj_t * create_scr_asic_status(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_asic_status(module);
    
    // Store reference to the status label
    asic_status_label = lv_obj_get_child(scr, 1);
    
    return scr;
}

static lv_obj_t * create_scr_configure(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    return display->create_scr_configure(module);
}

static lv_obj_t * create_scr_ota(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_ota(module);
    
    // Store references to the labels that need to update
    firmware_update_scr_filename_label = lv_obj_get_child(scr, 1);
    firmware_update_scr_status_label = lv_obj_get_child(scr, 2);
    
    return scr;
}

static lv_obj_t * create_scr_connection(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_connection(module);
    
    // Store reference to the status label
    wifi_status_label = lv_obj_get_child(scr, 1);
    
    return scr;
}

static lv_obj_t * create_scr_logo(const lv_img_dsc_t *img_dsc) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    return display->create_scr_logo(img_dsc);
}

static lv_obj_t * create_scr_urls(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_urls(module);
    
    // Store references to the labels that need to update
    mining_url_scr_urls_label = lv_obj_get_child(scr, 1);
    
    return scr;
}

static lv_obj_t * create_scr_stats() {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_stats();
    
    // Store references to the labels that need to update
    if (display == ssd1306_get_interface()) {
        // SSD1306 layout
        hashrate_label = lv_obj_get_child(scr, 0);
        efficiency_label = lv_obj_get_child(scr, 1);
        difficulty_label = lv_obj_get_child(scr, 2);
        chip_temp_label = lv_obj_get_child(scr, 3);
    } else {
        // SSD1309 layout (with containers)
        lv_obj_t *stats_container = lv_obj_get_child(scr, 1);
        lv_obj_t *row1 = lv_obj_get_child(stats_container, 0);
        hashrate_label = lv_obj_get_child(row1, 0);
        efficiency_label = lv_obj_get_child(row1, 1);
        difficulty_label = lv_obj_get_child(stats_container, 1);
        lv_obj_t *row3 = lv_obj_get_child(stats_container, 2);
        chip_temp_label = lv_obj_get_child(row3, 0);
        
        // Get shares and rejected labels from row4
        lv_obj_t *row4 = lv_obj_get_child(stats_container, 3);
        shares_label = lv_obj_get_child(row4, 0);
        rejected_label = lv_obj_get_child(row4, 1);
    }
    
    return scr;
}

static lv_obj_t * create_scr_wifi(SystemModule * module) {
    // Use the current display interface to create the screen
    const display_interface_t *display = get_current_display_interface();
    lv_obj_t *scr = display->create_scr_wifi(module);
    
    if (display == ssd1309_get_interface()) {
        ip_addr_scr_wifi_label = lv_obj_get_child(scr, 2);
        wifi_rssi_label = lv_obj_get_child(scr, 4);
    }

    return scr;
}

static void screen_show(screen_t screen)
{
    if (SCR_CAROUSEL_START > screen) {
        lv_display_trigger_activity(NULL);
    }

    if (current_screen != screen) {
        lv_obj_t * scr = screens[screen];

        if (scr && lvgl_port_lock(0)) {
            // Get the current display interface
            const display_interface_t *display = get_current_display_interface();
            
            // Use the animation duration from the display interface
            lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, display->anim_duration, 0, false);
            lvgl_port_unlock();
        }

        current_screen = screen;
        current_screen_time_ms = 0;
        current_screen_delay_ms = delays_ms[screen];
    }
}

static void screen_update_cb(lv_timer_t * timer)
{
    int32_t display_timeout_config = nvs_config_get_i32(NVS_CONFIG_DISPLAY_TIMEOUT, -1);

    if (0 > display_timeout_config) {
        // display always on
        display_on(true);
    } else if (0 == display_timeout_config) {
        // display off
        display_on(false);
    } else {
        // display timeout
        const uint32_t display_timeout = display_timeout_config * 60 * 1000;

        if ((lv_display_get_inactive_time(NULL) > display_timeout) && (SCR_CAROUSEL_START <= current_screen)) {
            display_on(false);
        } else {
            display_on(true);
        }
    }

    if (GLOBAL_STATE->SELF_TEST_MODULE.active) {

        screen_show(SCR_SELF_TEST);

        SelfTestModule * self_test = &GLOBAL_STATE->SELF_TEST_MODULE;

        if (self_test_message_label != NULL) {
            lv_label_set_text(self_test_message_label, self_test->message);
        }

        if (self_test->finished) {
            if (self_test->result) {
                if (self_test_result_label != NULL) {
                    lv_label_set_text(self_test_result_label, "TESTS PASS!");
                }
                if (self_test_finished_label != NULL) {
                    lv_label_set_text(self_test_finished_label, "Press RESET button to start Bitaxe.");
                }
            } else {
                if (self_test_result_label != NULL) {
                    lv_label_set_text(self_test_result_label, "TESTS FAIL!");
                }
                if (self_test_finished_label != NULL) {
                    lv_label_set_text(self_test_finished_label, "Hold BOOT button for 2 seconds to cancel self test, or press RESET to run again.");
                }
            }
        }

        return;
    }

    if (GLOBAL_STATE->SYSTEM_MODULE.is_firmware_update) {
        if (firmware_update_scr_filename_label != NULL &&
            strcmp(GLOBAL_STATE->SYSTEM_MODULE.firmware_update_filename, lv_label_get_text(firmware_update_scr_filename_label)) != 0) {
            lv_label_set_text(firmware_update_scr_filename_label, GLOBAL_STATE->SYSTEM_MODULE.firmware_update_filename);
        }
        if (firmware_update_scr_status_label != NULL &&
            strcmp(GLOBAL_STATE->SYSTEM_MODULE.firmware_update_status, lv_label_get_text(firmware_update_scr_status_label)) != 0) {
            lv_label_set_text(firmware_update_scr_status_label, GLOBAL_STATE->SYSTEM_MODULE.firmware_update_status);
        }
        screen_show(SCR_FIRMWARE_UPDATE);
        return;
    }

    SystemModule * module = &GLOBAL_STATE->SYSTEM_MODULE;

    if (module->asic_status) {
        if (asic_status_label != NULL) {
            lv_label_set_text(asic_status_label, module->asic_status);
        }
        screen_show(SCR_ASIC_STATUS);
        return;
    }

    if (module->overheat_mode == 1) {
        if (ip_addr_scr_overheat_label != NULL && strcmp(module->ip_addr_str, lv_label_get_text(ip_addr_scr_overheat_label)) != 0) {
            lv_label_set_text(ip_addr_scr_overheat_label, module->ip_addr_str);
        }
        screen_show(SCR_OVERHEAT);
        return;
    }

    if (module->ssid[0] == '\0') {
        screen_show(SCR_CONFIGURE);
        return;
    }

    if (module->ap_enabled) {
        if (wifi_status_label != NULL && strcmp(module->wifi_status, lv_label_get_text(wifi_status_label)) != 0) {
            lv_label_set_text(wifi_status_label, module->wifi_status);
        }
        screen_show(SCR_CONNECTION);
        current_screen_time_ms = 0;
        return;
    }

    // Carousel

    current_screen_time_ms += SCREEN_UPDATE_MS;

    PowerManagementModule * power_management = &GLOBAL_STATE->POWER_MANAGEMENT_MODULE;

    char *pool_url = module->is_using_fallback ? module->fallback_pool_url : module->pool_url;
    if (mining_url_scr_urls_label != NULL && strcmp(lv_label_get_text(mining_url_scr_urls_label), pool_url) != 0) {
        lv_label_set_text(mining_url_scr_urls_label, pool_url);
    }

    if (ip_addr_scr_urls_label != NULL && strcmp(lv_label_get_text(ip_addr_scr_urls_label), module->ip_addr_str) != 0) {
        lv_label_set_text(ip_addr_scr_urls_label, module->ip_addr_str);
    }
    
    // Update IP address on WIFI screen
    if (ip_addr_scr_wifi_label != NULL && strcmp(lv_label_get_text(ip_addr_scr_wifi_label), module->ip_addr_str) != 0) {
        lv_label_set_text(ip_addr_scr_wifi_label, module->ip_addr_str);
    }

    if (hashrate_label != NULL && current_hashrate != module->current_hashrate) {
        lv_label_set_text_fmt(hashrate_label, "Gh/s: %.2f", module->current_hashrate);
    }

    if (efficiency_label != NULL && (current_power != power_management->power || current_hashrate != module->current_hashrate)) {
        if (power_management->power > 0 && module->current_hashrate > 0) {
            float efficiency = power_management->power / (module->current_hashrate / 1000.0);
            lv_label_set_text_fmt(efficiency_label, "J/Th: %.2f", efficiency);
        }
    }

    if (module->FOUND_BLOCK && !found_block) {
        found_block = true;

        if (difficulty_label != NULL) {
            lv_obj_set_width(difficulty_label, LV_HOR_RES);
            lv_label_set_long_mode(difficulty_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
            lv_label_set_text_fmt(difficulty_label, "Best: %s   !!! BLOCK FOUND !!!", module->best_session_diff_string);
        }

        screen_show(SCR_STATS);
        lv_display_trigger_activity(NULL);
    } else {
        if (difficulty_label != NULL && current_difficulty != module->best_session_nonce_diff) {
            lv_label_set_text_fmt(difficulty_label, "Best: %s/%s", module->best_session_diff_string, module->best_diff_string);
        }
    }

    if (chip_temp_label != NULL && current_chip_temp != power_management->chip_temp_avg && power_management->chip_temp_avg > 0) {
        lv_label_set_text_fmt(chip_temp_label, "Temp: %.1f C", power_management->chip_temp_avg);
    }
    
    // Update shares and rejected shares
    if (shares_label != NULL) {
        lv_label_set_text_fmt(shares_label, "Shares: %llu", module->shares_accepted);
    }
    
    if (rejected_label != NULL) {
        lv_label_set_text_fmt(rejected_label, "Rej: %llu", module->shares_rejected);
    }
    
    // Update WIFI RSSI on WIFI screen
    if (wifi_rssi_label != NULL) {
        lv_label_set_text_fmt(wifi_rssi_label, "Signal: %d dBm", module->wifi_rssi);
    }

    current_hashrate = module->current_hashrate;
    current_power = power_management->power;
    current_difficulty = module->best_session_nonce_diff;
    current_chip_temp = power_management->chip_temp_avg;

    if (current_screen_time_ms <= current_screen_delay_ms || found_block) {
        return;
    }

    screen_next();
}

void screen_next()
{
    screen_show(current_screen == SCR_CAROUSEL_END ? SCR_CAROUSEL_START : current_screen + 1);
}

esp_err_t screen_start(void * pvParameters)
{
    GLOBAL_STATE = (GlobalState *) pvParameters;

    if (GLOBAL_STATE->SYSTEM_MODULE.is_screen_active) {
        SystemModule * module = &GLOBAL_STATE->SYSTEM_MODULE;

        screens[SCR_SELF_TEST] = create_scr_self_test();
        screens[SCR_OVERHEAT] = create_scr_overheat(module);
        screens[SCR_ASIC_STATUS] = create_scr_asic_status(module);
        screens[SCR_CONFIGURE] = create_scr_configure(module);
        screens[SCR_FIRMWARE_UPDATE] = create_scr_ota(module);
        screens[SCR_CONNECTION] = create_scr_connection(module);
        screens[SCR_BITAXE_LOGO] = create_scr_logo(&bitaxe_logo);
        screens[SCR_OSMU_LOGO] = create_scr_logo(&osmu_logo);
        screens[SCR_URLS] = create_scr_urls(module);
        screens[SCR_STATS] = create_scr_stats();
        screens[SCR_WIFI] = create_scr_wifi(module);

        lv_timer_create(screen_update_cb, SCREEN_UPDATE_MS, NULL);
    }

    return ESP_OK;
}
