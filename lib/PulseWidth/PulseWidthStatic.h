#ifndef __PULSE_WIDTH_STATIC_H__
#define __PULSE_WIDTH_STATIC_H__

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

/* Check an esp_err_t and return result if not ESP_OK */
#define pulseCHECK(func)                                                     \
  do {                                                                        \
    esp_err_t _err_chk = (func);                                              \
    if (_err_chk != ESP_OK) {                                                 \
      ESP_LOGE(buttonTAG, "Failed %s at %s:%i with code %i", #func, __FILE__, \
               __LINE__, _err_chk);                                           \
      return _err_chk;                                                        \
    }                                                                         \
  } while (0);

static void PulseWidthInitPulseCounter()
{
}

static void PulseWidthInitRemote()
{
}

static void PulseWidthInitInterrupt()
{
}

/**
 * @brief Adds a new button to the interrupt handler. Called for each newly
 * added button
 *
 * @param config    The config for the button being added
 *
 * @return esp_err_t
 */
static inline esp_err_t AddInterrupts(button_handler_t* handler)
{
    const button_config_t const* kConfig = &handler->button_config_;
    const gpio_num_t kGpio = kConfig->gpio_;

    pulseCHECK(
        gpio_isr_handler_add(kGpio, ButtonInterruptCallback, (void*)handler));
    pulseCHECK(gpio_set_intr_type(kGpio, kConfig->type_));
    pulseCHECK(gpio_intr_enable(kGpio));
    return ESP_OK;
}

/**
 * @brief Adds a new button to the interrupt handler. Called for each newly
 * added button
 *
 * @param config    The config for the button being added
 *
 * @return esp_err_t
 */
static inline esp_err_t AddInterrupts(button_handler_t* handler)
{
    const button_config_t const* kConfig = &handler->button_config_;
    const gpio_num_t kGpio = kConfig->gpio_;

    buttonCHECK(
        gpio_isr_handler_add(kGpio, ButtonInterruptCallback, (void*)handler));
    buttonCHECK(gpio_set_intr_type(kGpio, kConfig->type_));
    buttonCHECK(gpio_intr_enable(kGpio));
    return ESP_OK;
}

#endif