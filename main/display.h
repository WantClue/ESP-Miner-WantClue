#ifndef MAIN_DISPLAY_H
#define MAIN_DISPLAY_H

#include <stdio.h>
#include "esp_err.h"
#include "display_interface.h"

// Initialize the display
esp_err_t display_init(void * pvParameters);

// Turn the display on or off
esp_err_t display_on(bool display_on);

// Get the current display interface
const display_interface_t *get_current_display_interface(void);

#endif // MAIN_DISPLAY_H
