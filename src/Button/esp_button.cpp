#include "esp_button.h"

/**
 *
 * ISR bound to low or high level with pull resistor automatically set
 *
 * Interrupts disabled for given button in ISR for debouncing
 * Later processing requested using RTOS daemon / task
 *
 * Processor discriminates press vs release
 * Processor starts timer to re-enable interrupt after deboune period
 *
 *
 */

typedef struct {
  button_config_t *pxConfig;
  TickType_t xLastTickCount;
  bool bGpioLevel;

} button_handler_t;

TaskHandle_t xButtonTask, xDebounceTask;
QueueHandle_t xButtonQueue;
QueueHandle_t xDebounceQueue;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* The handler for GPIO interrupts */
static void vButtonInterruptHandler(void *pvParm) {
  portENTER_CRITICAL_ISR(&mux);
  button_config_t *pxButtonConfig = (button_config_t *)pvParm;
  if (pxButtonConfig->last_event - xTaskGetTickCountFromISR() <=
      pdMS_TO_TICKS(15)) {
    portEXIT_CRITICAL_ISR(&mux);
    return;
  }
  vButtonDebugFlip();
  gpio_set_intr_type(pxButtonConfig->gpio, GPIO_INTR_DISABLE);

  /* Request deferred processing */
  button_handler_t handler;
  handler.pxConfig = pxButtonConfig;
  handler.bGpioLevel = gpio_get_level(pxButtonConfig->gpio);
  handler.xLastTickCount = xTaskGetTickCountFromISR();

  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(xButtonQueue, (void *)&handler, &xHigherPriorityTaskWoken);
  portEXIT_CRITICAL_ISR(&mux);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

static bool bWasButtonPressed(button_handler_t *pxButton) {
  if (pxButton->bGpioLevel) {
    return pxButton->pxConfig->type == GPIO_INTR_HIGH_LEVEL;
  } else {
    return pxButton->pxConfig->type == GPIO_INTR_LOW_LEVEL;
  }
}

static void vButtonEventTask(void *pvParm) {
  button_handler_t xButtonQueued;

  for (;;) {
    /* Wait for button to be queued from ISR trigger*/
    xQueueReceive(xButtonQueue, (void *)&xButtonQueued, portMAX_DELAY);
    ESP_LOGI(buttonTAG, "Received button event");

    /* Discriminate button press vs release */
    if (bWasButtonPressed(&xButtonQueued)) {
      xButtonQueued.pxConfig->last_event = xButtonQueued.xLastTickCount;
    } else {
      xButtonQueued.xLastTickCount -= xButtonQueued.pxConfig->last_event;
      if (pdMS_TO_TICKS(xButtonQueued.xLastTickCount) >=
          xButtonQueued.pxConfig->hold_ms) {
        ESP_LOGI(buttonTAG, "HOLD");
        xButtonQueued.pxConfig->callback(BUTTON_HOLD);
      } else {
        ESP_LOGI(buttonTAG, "PRESS");
        xButtonQueued.pxConfig->callback(BUTTON_PRESS);
      }
    }

    xQueueSend(xDebounceQueue, (void *)&xButtonQueued, portMAX_DELAY);
  }
}

/* The timer used to re-enable interrupts after debounce period */
static void vDebounceEndTask(void *pvParm) {
  button_handler_t xButtonDequeued;
  ESP_LOGI(buttonTAG, "Started debounce handler task");

  for (;;) {
    /* Wait for button to be queued to exit debounce state */
    xQueueReceive(xDebounceQueue, (void *)&xButtonDequeued, portMAX_DELAY);
    ESP_LOGI(buttonTAG, "Received button debounce release IO %i",
             xButtonDequeued.pxConfig->gpio);

    /* Delay until it is time to exit debounce state */
    vTaskDelayUntil(&xButtonDequeued.xLastTickCount, 1);

    /* Set interrupt level to opposite of level that triggered interrupt */
    gpio_int_type_t eNewInterrupt;
    if (xButtonDequeued.bGpioLevel) {
      eNewInterrupt = GPIO_INTR_LOW_LEVEL;
      ESP_LOGI(buttonTAG, "Setting new interrupt to LOW_LEVEL");
    } else {
      eNewInterrupt = GPIO_INTR_HIGH_LEVEL;
      ESP_LOGI(buttonTAG, "Setting new interrupt to HIGH_LEVEL");
    }
    vButtonDebugFlip();
    vButtonDebugFlip();
    gpio_set_intr_type(xButtonDequeued.pxConfig->gpio, eNewInterrupt);
  }
}

esp_err_t xButtonDefaultConfig(button_config_t *pxConfig) {
  if (pxConfig == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  pxConfig->gpio = GPIO_NUM_MAX;
  pxConfig->hold_ms = buttonDEFAULT_HOLD_MS;
  return ESP_OK;
}

esp_err_t xButtonInit() {
  ESP_LOGI(buttonTAG, "Registering isr");

#ifdef BUTTON_DEBUG_PIN
  gpio_set_direction(BUTTON_DEBUG_PIN, GPIO_MODE_INPUT_OUTPUT);
#endif

  /* Create the task / queue debounce system */
  xButtonQueue = xQueueCreate(buttonQUEUE_SIZE, sizeof(button_handler_t));
  xDebounceQueue = xQueueCreate(buttonQUEUE_SIZE, sizeof(button_handler_t));
  xTaskCreate(vButtonEventTask, "but_event", 4096, NULL, 10, &xButtonTask);
  xTaskCreate(vDebounceEndTask, "but_bounce", 4096, NULL, 10, &xDebounceTask);

  /* Use gpio_install_isr_service() for per GPIO ISR */
  buttonCHECK(gpio_install_isr_service(buttonINTR_FLAGS));
  return ESP_OK;
}

esp_err_t xButtonAdd(button_config_t *pxConfig) {
  if (pxConfig == NULL) {
    ESP_LOGE(buttonTAG, "Couldn't add button, null pxConfig");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(buttonTAG, "Adding button on GPIO %i", pxConfig->gpio);

  /* Enable interrupt on GPIO */
  buttonCHECK(gpio_set_direction(pxConfig->gpio, GPIO_MODE_INPUT));
  buttonCHECK(gpio_set_pull_mode(pxConfig->gpio, GPIO_PULLUP_PULLDOWN));
  if (pxConfig->type == GPIO_INTR_LOW_LEVEL) {
    buttonCHECK(gpio_pullup_en(pxConfig->gpio));
  } else {
    buttonCHECK(gpio_pulldown_en(pxConfig->gpio));
  }

  /* Add button to ISR handler and create task */
  buttonCHECK(gpio_isr_handler_add(pxConfig->gpio, vButtonInterruptHandler,
                                   (void *)pxConfig));
  buttonCHECK(gpio_set_intr_type(pxConfig->gpio, pxConfig->type));
  buttonCHECK(gpio_intr_enable(pxConfig->gpio));
  return ESP_OK;
}

esp_err_t xButtonRemove(button_config_t *pxConfig) {
  if (pxConfig == NULL) {
    ESP_LOGE(buttonTAG, "Couldn't remove button, null pxConfig");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(buttonTAG, "Removing button on GPIO %i", pxConfig->gpio);
  return gpio_isr_handler_remove(pxConfig->gpio);
}

esp_err_t xButtonDeinit() {
  ESP_LOGI(buttonTAG, "Unregistering isr");
  gpio_uninstall_isr_service();
  vTaskDelete(xButtonTask);
  vTaskDelete(xDebounceTask);
  vQueueDelete(xButtonQueue);
  vQueueDelete(xDebounceQueue);
  return ESP_OK;
}
