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

static void task(void *parm) {
  while (true) {
  }
}

void cb(button_event_t event, int state) {
  ESP_LOGI("cb", "Start callback!!");
  ESP_LOGI("cb", "State %i", state);
}

void app_main() {
  ESP_ERROR_CHECK(esp_button_init());
  button_config_t config;
  esp_button_config(&config);
  config.gpio = GPIO_NUM_0;
  config.callback = &cb;
  config.type = GPIO_INTR_LOW_LEVEL;
  ESP_ERROR_CHECK(esp_add_button(&config));
  gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY);
  gpio_pullup_en(GPIO_NUM_0);
  xTaskCreate(task, "task", 2048, nullptr, 5, nullptr);
  while (true) {
  }
}
