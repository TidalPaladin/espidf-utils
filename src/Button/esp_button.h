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
#include "soc/gpio_reg.h"
#include "soc/soc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Check an esp_err_t and return result if not ESP_OK */
#define buttonCHECK(func)                                                     \
  do {                                                                        \
    esp_err_t _err_chk = (func);                                              \
    if (_err_chk != ESP_OK) {                                                 \
      ESP_LOGE(buttonTAG, "Failed %s at %s:%i with code %i", #func, __FILE__, \
               __LINE__, _err_chk);                                           \
      return _err_chk;                                                        \
    }                                                                         \
  } while (0);

#define eButtonIntrType(gpio) ((gpio_int_type_t)GPIO.pin[gpio].int_type)

#define BUTTON_DEBOUNCE_MS 30
#define BUTTON_HOLD_MS 3000
#define BUTTON_TASK_PRIORITY 10

/* Override the queue size for issues with multiple buttons firing at once */
#ifndef BUTTON_QUEUE_SIZE
#define BUTTON_QUEUE_SIZE 2
#endif

/* Manually set stack size */
#ifndef BUTTON_STACK_SIZE
#define BUTTON_STACK_SIZE (4096)
#endif

typedef enum {
  kButtonReleased = 0,
  kButtonTriggered = 1,
  kButtonPress = 2,
  kButtonHold = 3
} button_event_t;

typedef void (*button_callback_t)(button_event_t);

typedef struct {
  gpio_num_t gpio_;            /*!< GPIO to use for the button >*/
  gpio_int_type_t type_;       /*!< Ezdge to interrupt on >*/
  button_callback_t callback_; /*!< Callback to run on button press >*/
  uint32_t hold_ms_;           /*!< Milliseconds for press vs hold >*/
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
esp_err_t ButtonDefaultConfig(button_config_t* pxConfig);

/**
 * Installs the ISR service using gpio_install_isr_service()
 *
 * @return result of gpio_install_isr_service()
 */
esp_err_t ButtonInit();

/**
 * Create a new button instance. Supplied configuration variable will
 * be stored internally, so variable can be discarded after this call
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
esp_err_t ButtonAdd(button_config_t* pxConfig);

/**
 * Disable the button on a given GPIO
 *
 * @param gpio  The GPIO to disable button behavior on
 *              pre: gpio is attached to button with ButtonAdd
 *
 * @return esp_err_t
 *  - ESP_ERR_INVALID_ARG -> No button was active on the given GPIO
 *  - ESP_OK -> Success
 */
esp_err_t ButtonDisable(gpio_num_t gpio);

/**
 * Destroys a button instance
 *
 * @param config  The config for the button to be deactivated
 *
 * post: ISR deactivated and RTOS task deleted, RTOS stack space freed
 *
 * @return Result of gpio_isr_handler_remove()
 */
esp_err_t ButtonRemove(gpio_num_t gpio);

/**
 * Uninstalls the ISR service with gpio_uninstall_isr_service()
 *
 * @return ESP_OK
 */
esp_err_t ButtonDeinit();

#ifdef __cplusplus
}
#endif

#endif