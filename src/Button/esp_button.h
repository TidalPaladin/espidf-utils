#ifndef __ESP_BUTTON_H__
#define __ESP_BUTTON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define BUTTON_DEBUG_PIN (gpio_num_t)2

#define buttonTAG "button"
#define buttonINTR_FLAGS \
  ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_EDGE

#define buttonCHECK(func) do {esp_err_t r=(func); if(r != ESP_OK) return (r);}while(0)

#define buttonDEFAULT_DEBOUNCE_MS 5
#define buttonDEFAULT_HOLD_MS 3000
#define buttonDEFAULT_STACK_SIZE 4096
#define buttonDEFAULT_TASK_PRIORITY 10

#define buttonBIT_PRESSED 1

#ifndef buttonQUEUE_SIZE
#define buttonQUEUE_SIZE 2
#endif

typedef enum {
  BUTTON_RELEASED = 0,
  BUTTON_TRIGGERED = 1,
  BUTTON_PRESS = 2,
  BUTTON_HOLD = 3,
  BUTTON_MAX
} button_event_t;

typedef enum {
  BUTTON_PULL_NONE = 0,
  BUTTON_PULL_LOW,
  BUTTON_PULL_HIGH,
  BUTTON_PULL_MAX
} button_pull_t;



typedef void (*button_callback_t)(button_event_t);

typedef struct {
  gpio_num_t gpio;            /*!< GPIO to use for the button >*/
  gpio_int_type_t type;          /*!< Ezdge to interrupt on >*/
  button_callback_t callback; /*!< Callback to run on button press >*/
  uint32_t hold_ms;           /*!< Milliseconds for press vs hold >*/

} button_config_t;

/**
 * Initializes a button_config_t with default values
 *
 * @param config  Pointer to config to initialize
 *
 * post: config set to default values, gpio must be set manually
 *
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG on bad config
 */
esp_err_t xButtonDefaultConfig(button_config_t *pxConfig);

/**
 * Installs the ISR service using gpio_install_isr_service()
 *
 * @return result of gpio_install_isr_service()
 */
esp_err_t xButtonInit();

/**
 * Create a new button instance
 *
 * @param config  The button config to use for the new button
 *
 * post: ISR and RTOS task created for button
 *
 * @return
 *  - ESP_ERR_INVALID_ARG if config was bad
 *  - Other esp_err_t if one of the interrupt init commands failed
 *  - ESP_OK on success
 */
esp_err_t xButtonAdd(button_config_t *pxConfig);

/**
 * Destroys a button instance
 *
 * @param config  The config for the button to be deactivated
 *
 * post: ISR deactivated and RTOS task deleted, RTOS stack space freed
 *
 * @return Result of gpio_isr_handler_remove()
 */
esp_err_t xButtonRemove(button_config_t *pxConfig);

/**
 * Uninstalls the ISR service with gpio_uninstall_isr_service()
 *
 * @return ESP_OK
 */
esp_err_t xButtonDeinit();

#endif