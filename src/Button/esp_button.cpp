#include "esp_button.h"

const char *TAG = "button";

static void button_isr(void *parm);

static void button_task(void *parm);

static button_event_t get_event_from_gpio_state(
    const button_config_t *const config, bool gpio_state) {
  /* POSEDGE trigger means GPIO LOW is button pressed */
  if (config->type == BUTTON_INTR_POSEDGE) {
    return gpio_state ? BUTTON_RELEASED : BUTTON_TRIGGERED;
  } else {
    return gpio_state ? BUTTON_TRIGGERED : BUTTON_RELEASED;
  }
}

static void set_button_pull_mode(const button_config_t *const config) {
  switch (config->pull) {
    case BUTTON_PULL_LOW:
      gpio_set_pull_mode(config->gpio, GPIO_PULLDOWN_ONLY);
      gpio_pulldown_en(GPIO_NUM_0);
      break;
    case BUTTON_PULL_HIGH:
      gpio_set_pull_mode(config->gpio, GPIO_PULLUP_ONLY);
      gpio_pullup_en(GPIO_NUM_0);
      break;
    default:
      break;
  }
}

esp_err_t esp_button_config(button_config_t *config) {
  if (config == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  config->gpio = GPIO_NUM_MAX;
  config->debounce_ms = BUTTON_DEFAULT_DEBOUNCE_MS;
  config->hold_ms = BUTTON_DEFAULT_HOLD_MS;
  config->cb_stack_size = BUTTON_DEFAULT_STACK_SIZE;
  config->cb_task_priority = BUTTON_DEFAULT_TASK_PRIORITY;
  config->pull = BUTTON_PULL_NONE;
  return ESP_OK;
}

esp_err_t esp_button_init() {
  ESP_LOGI(TAG, "Registering isr");

#ifdef BUTTON_DEBUG_PIN
  gpio_set_direction(BUTTON_DEBUG_PIN, GPIO_MODE_OUTPUT);
#endif

  /* Use gpio_install_isr_service() for per GPIO ISR */
  return gpio_install_isr_service(BUTTON_INTR_FLAGS);
}

esp_err_t esp_add_button(button_config_t *config) {
  if (config == NULL) {
    ESP_LOGE(TAG, "Couldn't add button, null config");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Adding button on GPIO %i", config->gpio);

  /* Enable interrupt on GPIO */
  esp_err_t result = gpio_set_direction(config->gpio, GPIO_MODE_INPUT);

  set_button_pull_mode(config);

  /* Add button to ISR handler and create task */
  result = gpio_isr_handler_add(config->gpio, button_isr, (void *)config);
  xTaskCreate(button_task, "button_master", config->cb_stack_size,
              (void *)config, config->cb_task_priority, &config->task);
  result = gpio_set_intr_type(config->gpio, GPIO_INTR_ANYEDGE);
  result = gpio_intr_enable(config->gpio);
  return result;
}

esp_err_t esp_remove_button(button_config_t *config) {
  if (config == NULL) {
    ESP_LOGE(TAG, "Couldn't remove button, null config");
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "Removing button on GPIO %i", config->gpio);
  vTaskDelete(config->task);
  return gpio_isr_handler_remove(config->gpio);
}

esp_err_t esp_button_deinit() {
  ESP_LOGI(TAG, "Unregistering isr");
  gpio_uninstall_isr_service();
  return ESP_OK;
}

static void button_isr(void *parm) {
  /* Hold GPIO state to be read in task */
  const button_config_t *const config = (button_config_t *)parm;
  const uint8_t BIT = gpio_get_level(config->gpio);
  BaseType_t xHigherPriorityTaskWoken;

  /* Notify task and leave ISR */
  xTaskNotifyFromISR(config->task, BIT, eSetValueWithOverwrite,
                     &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

static void button_task(void *parm) {
  const button_config_t *const config = (button_config_t *)parm;
  button_event_t event = BUTTON_RELEASED;
  uint32_t ulTaskNotification = 0;
  TickType_t xTicksElapsed = 0;

  while (true) {
    xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &ulTaskNotification, portMAX_DELAY);

#ifdef BUTTON_DEBUG_PIN
    gpio_set_level(BUTTON_DEBUG_PIN, ulTaskNotification);
#endif

    event = get_event_from_gpio_state(config, ulTaskNotification);
    const bool NEW_GPIO = gpio_get_level(config->gpio);

    switch (event) {
      case BUTTON_TRIGGERED:
        xTicksElapsed = xTaskGetTickCount();
        break;
      case BUTTON_RELEASED:
        xTicksElapsed = xTaskGetTickCount() - xTicksElapsed;
        if (xTicksElapsed * portTICK_PERIOD_MS >= config->hold_ms) {
          ESP_LOGI(TAG, "Detected button hold");
          config->callback(BUTTON_HOLD);
        } else {
          ESP_LOGI(TAG, "Detected button press");
          config->callback(BUTTON_PRESS);
        }
        break;
    }
    vTaskDelay(pdMS_TO_TICKS(config->debounce_ms));
    }
}
