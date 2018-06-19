#include "DNS/DNS.h"
#include "SmartConfig/EasyWifi.h"
#include "SmartConfig/SmartConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void dns_resolve_task(void *parm) {
  EasyWifi::wait_for_wifi(20);
  ip_addr_t result;
  DNS::resolve("tidalpaladin.com", &result);
  ESP_LOGI("MAIN", "DNS resolve done");
  vTaskDelete(NULL);
}

void connect_wifi() { EasyWifi::connect(); }

void app_main() {
  EasyWifi::init_hardware();
  EasyWifi::init_software();
  uint32_t timeout_s = 120;
  connect_wifi();
  // SmartConfig::sc_start(&timeout_s);
  xTaskCreate(dns_resolve_task, "task", 2048, nullptr, 5, nullptr);
}
