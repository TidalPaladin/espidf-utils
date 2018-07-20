#include "DNS/DNS.h"
#include "SmartConfig/EasyWifi.h"
#include "SmartConfig/SmartConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Button/esp_button.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/inet.h"

extern "C" {
void app_main();
}

static void task(void* parm) {
  while (true) {
    vTaskDelay(portMAX_DELAY);
  }
}

static void buttonCallback(button_event_t event) {
  ESP_LOGI("cb", "Start callback!!");
}

void app_main() {
  ESP_ERROR_CHECK(ButtonInit());
  button_config_t config;
  ButtonDefaultConfig(&config);
  config.gpio_ = GPIO_NUM_0;
  config.callback_ = buttonCallback;
  config.type_ = GPIO_INTR_LOW_LEVEL;
  ESP_ERROR_CHECK(ButtonAdd(&config));
  config.callback_(kButtonHold);

  xTaskCreate(task, "task", 2048, nullptr, 5, nullptr);
  while (true) {
    vTaskDelay(portMAX_DELAY);
  }
}
