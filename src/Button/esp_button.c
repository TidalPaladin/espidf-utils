#include "esp_button.h"
#include "button_static.h"

/**
 *
 * NOTE:
 * Use of low / high level interrupts is preferred to edge interrupts. This
 * prevents situations where the button may think it is in one state, but
 * actually in another. Coupling to the level, rather than an edge, means that
 * the button will always know if it is pressed or released
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

gpio_int_type_t SetOppositeInterruptType(button_handler_t* button) {
  assert(button != NULL && "Null button pointer");
  assert(button->current_interrupt_type_ != GPIO_INTR_DISABLE);

  gpio_int_type_t new_interrupt;
  if (button->current_interrupt_type_ == GPIO_INTR_LOW_LEVEL) {
    new_interrupt = GPIO_INTR_HIGH_LEVEL;
  } else {
    new_interrupt = GPIO_INTR_LOW_LEVEL;
  }
  button->current_interrupt_type_ = new_interrupt;
  return new_interrupt;
}

void ButtonInterruptCallback(void* pvParm) {
  portENTER_CRITICAL_ISR(&mux);

  /* Flip interrupt type_ */
  button_handler_t* button = (button_handler_t*)pvParm;
  button_config_t* config = &button->button_config_;
  gpio_set_intr_type(config->gpio_, GPIO_INTR_DISABLE);

  SetOppositeInterruptType(button);
  const gpio_int_type_t depress_interrupt_type = config->type_;
  const gpio_int_type_t last_interrupt_type = button->current_interrupt_type_;

  if (last_interrupt_type != depress_interrupt_type) {
    ButtonDebugSetLevel(1);
    button->tick_count_press_ = xTaskGetTickCountFromISR();
    portEXIT_CRITICAL_ISR(&mux);
    gpio_set_intr_type(config->gpio_, button->current_interrupt_type_);
  } else {
    ButtonDebugSetLevel(0);
    button->tick_count_release_ = xTaskGetTickCountFromISR();

    /* Push button to task queue for deferred processing */
    BaseType_t xHigherPriorityTaskWoken;
    xQueueSendFromISR(kButtonQueue, (void*)&button, &xHigherPriorityTaskWoken);
    /* If we woke a higher priority task from ISR, exit ISR to that task */
    if (xHigherPriorityTaskWoken == pdTRUE) {
      portEXIT_CRITICAL_ISR(&mux);
      portYIELD_FROM_ISR();
    }
  }
}

uint32_t GetPressDurationMillis(button_handler_t* button) {
  return portTICK_PERIOD_MS *
         (button->tick_count_release_ - button->tick_count_press_);
}

void ButtonProcessRelease(button_handler_t* button) {
  const uint32_t kElapsedMs =
      (button->tick_count_release_ - button->tick_count_press_) *
      portTICK_PERIOD_MS;
  static uint32_t hit_count = 0;

  // TODO give variable name to debounce threshold
  if (kElapsedMs < BUTTON_DEBOUNCE_MS) {
    // NO-OP, pulse too short
    ESP_LOGD(buttonTAG, "Ignoring short pulse - %i ms", kElapsedMs);
    gpio_set_intr_type(button->button_config_.gpio_,
                       button->current_interrupt_type_);
    return;
  }

  /* Pulse was of valid length, run press / hold callback */
  hit_count++;
  if (kElapsedMs >= button->button_config_.hold_ms_) {
    ESP_LOGI(buttonTAG, "HOLD - %i ms", kElapsedMs);
    button->button_config_.callback_(kButtonHold);
  } else {
    ESP_LOGI(buttonTAG, "PRESS - %i ms", kElapsedMs);
    button->button_config_.callback_(kButtonPress);
  }
  gpio_set_intr_type(button->button_config_.gpio_,
                     button->current_interrupt_type_);
}

void ButtonEventTask(void* pvParm) {
  button_handler_t* button;

  /* Begin blocking loop */
  while (1) {
    /* Wait for button to be queued from ISR trigger*/
    BaseType_t got_item_from_queue = pdFALSE;
    got_item_from_queue =
        xQueueReceive(kButtonQueue, (void*)&button, portMAX_DELAY);

    if (got_item_from_queue == pdFALSE) {
      ESP_LOGE(buttonTAG, "xQueueReceive expired without getting anything");
      continue;
    }

    assert(button != NULL);
    ButtonProcessRelease(button);
  }
}

esp_err_t ButtonDefaultConfig(button_config_t* button_config) {
  if (button_config == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  button_config->gpio_ = GPIO_NUM_MAX;
  button_config->hold_ms_ = BUTTON_HOLD_MS;
  return ESP_OK;
}

esp_err_t ButtonInit() {
  ESP_LOGI(buttonTAG, "Registering isr");

#ifdef BUTTON_DEBUG_PIN
  gpio_set_direction(BUTTON_DEBUG_PIN, GPIO_MODE_INPUT_OUTPUT);
#endif

  /* Create the task / queue debounce system */
  kButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(button_handler_t*));
  xTaskCreate(ButtonEventTask, "but_event", 4096, NULL, 10, &kButtonTask);

  /* Use gpio_install_isr_service() for per GPIO ISR */
  buttonCHECK(gpio_install_isr_service(kButtonIntrFlags));
  return ESP_OK;
}

esp_err_t ButtonAdd(button_config_t* button_config) {
  if (button_config == NULL) {
    ESP_LOGE(buttonTAG, "Couldn't add button, null button_config");
    return ESP_ERR_INVALID_ARG;
  }
  // ESP_LOGI(buttonTAG, "Adding button on GPIO %i",
  // (uint8_t)button_config->gpio_);

  /* Allocate and store button info */
  button_handler_t* const handler = (button_handler_t*)malloc(sizeof(*handler));
  memcpy((void*)&handler->button_config_, (void*)button_config,
         sizeof(*button_config));
  handler->current_interrupt_type_ = button_config->type_;

  /* Enable interrupt on GPIO */
  buttonCHECK(gpio_set_direction(button_config->gpio_, GPIO_MODE_INPUT));
  buttonCHECK(gpio_set_pull_mode(button_config->gpio_, GPIO_PULLUP_PULLDOWN));
  if (button_config->type_ == GPIO_INTR_LOW_LEVEL) {
    buttonCHECK(gpio_pullup_en(button_config->gpio_));
  } else {
    buttonCHECK(gpio_pulldown_en(button_config->gpio_));
  }

  /* Add button to ISR handler and create task */
  buttonCHECK(gpio_isr_handler_add(button_config->gpio_,
                                   ButtonInterruptCallback, (void*)handler));
  buttonCHECK(gpio_set_intr_type(button_config->gpio_, button_config->type_));
  buttonCHECK(gpio_intr_enable(button_config->gpio_));
  return ESP_OK;
}

esp_err_t ButtonRemove(const gpio_num_t gpio_) {
  ESP_LOGI(buttonTAG, "Removing button on GPIO %i", gpio_);
  buttonCHECK(gpio_set_intr_type(gpio_, GPIO_INTR_DISABLE));
  buttonCHECK(gpio_intr_disable(gpio_));
  buttonCHECK(gpio_isr_handler_remove(gpio_));
  return ESP_OK;
}

esp_err_t ButtonDeinit() {
  ESP_LOGI(buttonTAG, "Unregistering isr");
  gpio_uninstall_isr_service();
  vTaskDelete(kButtonTask);
  vQueueDelete(kButtonQueue);
  return ESP_OK;
}
