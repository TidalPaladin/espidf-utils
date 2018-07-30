#ifndef __PULSE_WIDTH_H__
#define __PULSE_WIDTH_H__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "gpio.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcnt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

typedef void (*pulse_width_cb_t)(gpio_num_t, uint32_t);

typedef struct {
    pulse_width_cb_t callback_;
    gpio_num_t gpio_;
    uint32_t min_pulse_us_;
    uint32_t max_pulse_us_;
    gpio_int_type_t type_;
} pulse_width_config_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t PulseWidthInit();

esp_err_t PulseWidthAdd(pulse_width_config_t* config);

esp_err_t PulseWidthRemove(gpio_num_t* gpio);

#ifdef __cplusplus
}
#endif

#endif