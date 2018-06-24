#ifndef __ESP_BUTTON_H__
#define __ESP_BUTTON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

typedef enum {
  BUTTON_PRESS = 0,
  BUTTON_HOLD,
  BUTTON_BOUNCE,
  BUTTON_MAX
} button_event_t;

typedef void (*button_callback_t)(button_event_t, int);

typedef struct {
  gpio_num_t gpio;
  gpio_int_type_t type;
  button_callback_t callback;
  uint32_t debounce_ms;
  uint32_t hold_ms;
} button_config_t;

esp_err_t esp_button_config(button_config_t *config);

esp_err_t esp_button_init();

esp_err_t esp_add_button(button_config_t *config);

esp_err_t esp_remove_button(button_config_t *config);

esp_err_t esp_button_deinit();

#endif