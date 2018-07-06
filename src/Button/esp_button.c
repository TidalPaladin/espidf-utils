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

esp_err_t ButtonDefaultConfig(button_config_t* button_config) {
  assert(button_config != NULL);
  button_config->gpio_ = GPIO_NUM_MAX;
  button_config->hold_ms_ = BUTTON_HOLD_MS;
  return ESP_OK;
}

esp_err_t ButtonInit() {
  ESP_LOGI(buttonTAG, "Registering isr");

  assert(BUTTON_STACK_SIZE >= configMINIMAL_STACK_SIZE &&
         BUTTON_QUEUE_SIZE > 0);

  kButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(button_handler_t*));
  xTaskCreate(ButtonEventTask, "but_event", 1024, NULL, 10, &kButtonTask);

  assert(kButtonQueue != NULL && kButtonTask != NULL);

  /* Use gpio_install_isr_service() for per GPIO ISR */
  buttonCHECK(gpio_install_isr_service(kButtonIntrFlags));
  return ESP_OK;
}

esp_err_t ButtonAdd(button_config_t* button_config) {
  assert(button_config != NULL);
  ESP_LOGI(buttonTAG, "Adding button on GPIO %i",
           (uint8_t)button_config->gpio_);

  button_handler_t* handler = CopyConfigToHeap(button_config);
  assert(handler != NULL);

  buttonCHECK(gpio_set_direction(button_config->gpio_, GPIO_MODE_INPUT));
  buttonCHECK(SetPullResistors(button_config));
  buttonCHECK(AddInterruptsForButton(handler));
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
  assert(kButtonTask == NULL && kButtonQueue == NULL);
  return ESP_OK;
}

/**
 *
 *
 * Begin static methods
 *
 *
 */

gpio_int_type_t UpdateInterruptType(button_handler_t* button) {
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

  /* Disable interrupts ASAP or interrupts will continue to queue */
  button_handler_t* button = (button_handler_t*)pvParm;
  button_config_t* config = &button->button_config_;
  gpio_set_intr_type(config->gpio_, GPIO_INTR_DISABLE);

  /* Released if ISR trigger doesn't match user defined press interrupt type */
  const bool kWasReleased = (button->current_interrupt_type_ != config->type_);
  UpdateInterruptType(button);

  /* TODO rename variables, this is confusing */
  BaseType_t needs_task_switch;
  if (kWasReleased) {
    needs_task_switch = ButtonIsrHandleRelease(button);
  } else {
    needs_task_switch = ButtonIsrHandleDepress(button);
  }

  /* If we woke a higher priority task from ISR, exit ISR to that task */
  portEXIT_CRITICAL_ISR(&mux);
  if (needs_task_switch == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

BaseType_t ButtonIsrHandleDepress(button_handler_t* handler) {
  handler->tick_count_press_ = xTaskGetTickCountFromISR();

  /* Enable interrupts upon exit of ISR for button depress */
  gpio_set_intr_type(handler->button_config_.gpio_,
                     handler->current_interrupt_type_);
  return pdFALSE;
}

BaseType_t ButtonIsrHandleRelease(volatile button_handler_t* handler) {
  handler->tick_count_release_ = xTaskGetTickCountFromISR();

  /* Push button to task for deferred processing of release */
  BaseType_t higher_prio_task_woken = pdFALSE;
  xQueueSendFromISR(kButtonQueue, (void*)&handler, &higher_prio_task_woken);
  return higher_prio_task_woken;
}

uint32_t GetPressDurationMillis(button_handler_t* button) {
  return portTICK_PERIOD_MS *
         (button->tick_count_release_ - button->tick_count_press_);
}

void ButtonProcessRelease(button_handler_t* button) {
  assert(button != NULL);

  const uint32_t kElapsedMs =
      (button->tick_count_release_ - button->tick_count_press_) *
      portTICK_PERIOD_MS;

  if (kElapsedMs < BUTTON_DEBOUNCE_MS) {
    // NO-OP, pulse too short
    ESP_LOGD(buttonTAG, "Ignoring short pulse - %i ms", kElapsedMs);
    return;
  }

  /* Pulse was of valid length, run press / hold callback */
  assert(button->button_config_.callback_ != NULL);
  if (kElapsedMs >= button->button_config_.hold_ms_) {
    ESP_LOGI(buttonTAG, "HOLD - %i ms", kElapsedMs);
    button->button_config_.callback_(kButtonHold);
  } else {
    ESP_LOGI(buttonTAG, "PRESS - %i ms", kElapsedMs);
    button->button_config_.callback_(kButtonPress);
  }
}

void ButtonEventTask(void* pvParm /* Not used */) {
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

    /* Re-enable enterrupt based on value set from UpdateInterruptType() */
    gpio_set_intr_type(button->button_config_.gpio_,
                       button->current_interrupt_type_);
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
    // ESP_LOGI(buttonTAG, "Stack watermark: %i words",
    //          (uint32_t)uxTaskGetStackHighWaterMark(NULL));
#endif
  }
}

button_handler_t* CopyConfigToHeap(const button_config_t* const config) {
  const size_t kSize = sizeof(button_handler_t);
  button_handler_t* handler = (button_handler_t*)malloc(kSize);
  assert(handler != NULL);

  void* dest = &handler->button_config_;
  const void* kSrc = config;
  memcpy(dest, kSrc, sizeof(*config));

  handler->current_interrupt_type_ = config->type_;
  return handler;
}

esp_err_t SetPullResistors(const button_config_t* const config) {
  buttonCHECK(gpio_set_pull_mode(config->gpio_, GPIO_PULLUP_PULLDOWN));
  if (config->type_ == GPIO_INTR_LOW_LEVEL) {
    buttonCHECK(gpio_pullup_en(config->gpio_));
  } else {
    buttonCHECK(gpio_pulldown_en(config->gpio_));
  }
  return ESP_OK;
}

esp_err_t AddInterruptsForButton(button_handler_t* handler) {
  button_config_t* config = &handler->button_config_;

  buttonCHECK(gpio_isr_handler_add(config->gpio_, ButtonInterruptCallback,
                                   (void*)handler));
  buttonCHECK(gpio_set_intr_type(config->gpio_, config->type_));
  buttonCHECK(gpio_intr_enable(config->gpio_));
  return ESP_OK;
}
