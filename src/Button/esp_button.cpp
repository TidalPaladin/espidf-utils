#include "esp_button.h"

const char *TAG = "button";

static void button_isr(void *parm);
static uint32_t get_elapsed_time_ms(TickType_t *);

esp_err_t esp_button_config(button_config_t *config) {
  config->gpio = GPIO_NUM_MAX;
  config->type = GPIO_INTR_ANYEDGE;
  config->debounce_ms = 30;
  config->hold_ms = 3000;
  return ESP_OK;
}

esp_err_t esp_button_init() {
  ESP_LOGI(TAG, "Registering isr");
  return gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
}

esp_err_t esp_add_button(button_config_t *config) {
  ESP_LOGI(TAG, "Adding button on GPIO %i", config->gpio);
  config->last_event = xTaskGetTickCount();
  gpio_set_direction(config->gpio, GPIO_MODE_INPUT);
  gpio_intr_enable(config->gpio);
  gpio_set_intr_type(config->gpio, config->type);
  return gpio_isr_handler_add(config->gpio, button_isr, (void *)config);
}

esp_err_t esp_remove_button(button_config_t *config) {
  ESP_LOGI(TAG, "Removing button on GPIO %i", config->gpio);
  return gpio_isr_handler_remove(config->gpio);
}

esp_err_t esp_button_deinit() {
  ESP_LOGI(TAG, "Unregistering isr");
  gpio_uninstall_isr_service();
  return ESP_OK;
}

void button_isr(void *parm) {
  /* Record GPIO state and time elapsed since last event */
  button_config_t *config = (button_config_t *)parm;
  const int LEVEL = gpio_get_level(config->gpio);
  const uint32_t ELASPED_MS = get_elapsed_time_ms(&config->last_event);
  config->last_event = xTaskGetTickCountFromISR();

  /* Discriminate bounce / press / hold */
  button_event_t event_type;
  if (ELASPED_MS >= config->hold_ms) {
    event_type = BUTTON_HOLD;
  } else if (ELASPED_MS < config->debounce_ms) {
    event_type = BUTTON_BOUNCE;
  } else {
    event_type = BUTTON_PRESS;
  }

  ESP_LOGI(TAG, "Received interrupt on GPIO%i", config->gpio);
  ESP_LOGI(TAG, "Elasped ms since last event: %i", ELASPED_MS);

  /* TODO handle this with task create? */
  (config->callback)(event_type, LEVEL);
}

uint32_t get_elapsed_time_ms(TickType_t *start) {
  return portTICK_PERIOD_MS * (xTaskGetTickCount() - *start);
}