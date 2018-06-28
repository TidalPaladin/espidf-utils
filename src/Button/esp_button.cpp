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

} button_handler_t;

QueueHandle_t xDebounceQueue;

/* The handler for GPIO interrupts */
static void vButtonInterruptHandler(void *pvParm);

/* The deferred handler that processes interrupts after the interrupt */
static void vDeferredHandlingFunction(void *pvParm, uint32_t ulParm);

/* The timer used to re-enable interrupts after debounce period */
static void vTimerCallback(TimerHandle_t xTimer) {
  button_config_t *xButtonDequeued;
  
  /* Process all queued buttons that need ISR modifications */
  BaseType_t xQueueItemReceived = xQueueReceive(xDebounceQueue, (void*)xButtonDequeued, 0);
  while(xQueueItemReceived == pdTRUE) {
    gpio_set_intr_type(xButtonDequeued->gpio, xButtonDequeued->type);
    xQueueItemReceived = xQueueReceive(xDebounceQueue, (void*)&xButtonDequeued, 0);
  }
}

/* Called from the deferred handler to process button release */
static void vButtonHandleRelease(button_config_t *pxButtonConfig, TickType_t xNewTickCount) {
    const uint32_t xTicksElapsed = xNewTickCount - pxButtonConfig->last_wake;
    const uint32_t xTicksHoldThreshold = pdMS_TO_TICKS(pxButtonConfig->hold_ms);
    
    /* Discriminate press vs hold based on duration */
    button_event_t xEvent;
    if(xTicksElapsed >= xTicksHoldThreshold) {
      xEvent = BUTTON_PRESS;
    } else {
      xEvent = BUTTON_HOLD;
    }

    ESP_LOGI(buttonTAG, "Button %s on IO %i",
             xEvent == BUTTON_PRESS ? "press" : "hold", pxButtonConfig->gpio);
    pxButtonConfig->callback(xEvent);
}

static void vButtonHandlePress(button_config_t *pxButtonConfig, TickType_t xNewTickCount) {
  pxButtonConfig->last_wake = xNewTickCount;
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
  gpio_set_direction(BUTTON_DEBUG_PIN, GPIO_MODE_OUTPUT);
#endif

  /* Create the timer / queue debounce system */
  xDebounceQueue = xQueueCreate(buttonQUEUE_SIZE, sizeof(button_config_t*));
  xTimerCreate("debounce", pdMS_TO_TICKS(10), pdFALSE, NULL, vTimerCallback);

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
  if(pxConfig->type == GPIO_INTR_NEGEDGE) {
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
  return ESP_OK;
}

static void vButtonInterruptHandler(void *pvParm) {
  /* Latch GPIO as debounce mechanism */
  button_config_t *pxButtonConfig = (button_config_t *)pvParm;
  gpio_pad_hold(pxButtonConfig->gpio);
  gpio_set_intr_type(pxButtonConfig->gpio, GPIO_INTR_DISABLE);

#ifdef BUTTON_DEBUG_PIN
  static bool level = false;
  gpio_set_level(BUTTON_DEBUG_PIN, level);
  level = !level;
#endif

  /* Request deferred processing */
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTimerPendFunctionCallFromISR(vDeferredHandlingFunction, pvParm,
                                xTaskGetTickCountFromISR(),
                                &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

static void vDeferredHandlingFunction(void *pvParm, uint32_t ulParm) {
  button_config_t *pxButtonConfig = (button_config_t *)pvParm;
  const bool bGpioLevel = gpio_get_level(pxButtonConfig->gpio);
  gpio_pad_unhold(pxButtonConfig->gpio);
  gpio_int_type_t xNewInterruptType;

  if(bGpioLevel && (pxButtonConfig->type == GPIO_INTR_POSEDGE)) {
    /* Button depressed */
    pxButtonConfig->last_wake = ulParm;
    xNewInterruptType = GPIO_INT
  } else {
    /* Button released */
    xNewInterruptType = pxButtonConfig->type;
  }

  gpio_set_intr_type(pxButtonConfig->gpio, GPIO_INTR_ANYEDGE);
}