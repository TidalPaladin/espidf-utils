#ifdef UNIT_TEST
#include "Button/esp_button.h"
#include "unity.h"

#define BUTTON_PIN 0

void callback() {}

void init() {
  button_config_t config;
  ButtonDefaultConfig(&config);
  ButtonInit();
  config.gpio_ = BUTTON_PIN;
  config.callback_ = callback;
  config.type_ = GPIO_INTR_LOW_LEVEL;
  ButtonAdd(&config);
}

void press_test(void *) {}

void test_task(void *) {
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  RUN_TEST(ms_to_ticks);
  RUN_TEST(ticks_to_ms);
  RUN_TEST(delay);
  // RUN_TEST(delay_until_ms);

  vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif