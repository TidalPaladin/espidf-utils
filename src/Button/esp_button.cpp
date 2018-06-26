#include "esp_button.h"

const char *TAG = "button";

static void button_isr(void *parm);

static void button_task(void *parm);

esp_err_t esp_button_config(button_config_t *config) {
  config->gpio = GPIO_NUM_MAX;
  config->type = GPIO_INTR_ANYEDGE;
  config->debounce_ms = 30;
  config->hold_ms = 3000;
  config->cb_stack_size = 4096;
  return ESP_OK;
}

esp_err_t esp_button_init() {
  ESP_LOGI(TAG, "Registering isr");
  return gpio_install_isr_service(ESP_INTR_FLAG_EDGE | ESP_INTR_FLAG_IRAM);
}

esp_err_t esp_add_button(button_config_t *config) {
  ESP_LOGI(TAG, "Adding button on GPIO %i", config->gpio);

  esp_err_t result = gpio_set_direction(config->gpio, GPIO_MODE_INPUT);
  if (result != ESP_OK) return result;

  result = gpio_intr_enable(config->gpio);
  if (result != ESP_OK) return result;

  gpio_set_intr_type(config->gpio, config->type);
  if (result != ESP_OK) return result;

  result = gpio_isr_handler_add(config->gpio, button_isr, (void *)config);
  xTaskCreate(button_task, "button_master", config->cb_stack_size,
              (void *)config, 10, &config->task);
  return result;
}

esp_err_t esp_remove_button(button_config_t *config) {
  ESP_LOGI(TAG, "Removing button on GPIO %i", config->gpio);
  vTaskDelete(config->task);
  return gpio_isr_handler_remove(config->gpio);
}

esp_err_t esp_button_deinit() {
  ESP_LOGI(TAG, "Unregistering isr");
  gpio_uninstall_isr_service();
  return ESP_OK;
}

void button_isr(void *parm) {
  BaseType_t xHigherPriorityTaskWoken;
  button_config_t *config = (button_config_t *)parm;
  vTaskNotifyGiveFromISR(config->task, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}

static void button_task(void *parm) {
  const button_config_t *const config = (button_config_t *)parm;
  const TickType_t ticks_to_wait = pdMS_TO_TICKS(config->hold_ms);
  TickType_t ticks_elapsed;

  while (true) {
    /* Block here waiting for button press */
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

    /* Button pressed, measure time to release or timeout */
    ticks_elapsed = xTaskGetTickCount();
    ESP_LOGI(TAG, "Button pressed on GPIO %i, waiting for release",
             config->gpio);
    bool button_released = ulTaskNotifyTake(pdTRUE, ticks_to_wait);
    ticks_elapsed = xTaskGetTickCount() - ticks_elapsed;
    ESP_LOGI(TAG, "Button released after %i ms",
             ticks_elapsed * portTICK_PERIOD_MS);

    /* Determine press vs hold */
    if (ticks_elapsed * portTICK_PERIOD_MS >= config->hold_ms) {
      ESP_LOGI(TAG, "Detected button hold");
      config->callback(BUTTON_HOLD);
    } else {
      ESP_LOGI(TAG, "Detected button press");
      config->callback(BUTTON_PRESS);
    }

    /* If we timed out waiting for release, block here until user releases
     * button */
    if (!button_released) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    /* Delay for debounce */
    vTaskDelay(pdMS_TO_TICKS(config->debounce_ms));
  }
}