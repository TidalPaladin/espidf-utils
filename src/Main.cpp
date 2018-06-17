#include "DNS/DNS.h"
#include "SmartConfig/SmartConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/dns.h"
#include "lwip/inet.h"

extern "C" {
void app_main();
}

static void task(void* param) {
  ESP_LOGI("DEF", "STARTING!!!\n");
  ip_addr_t result = DNS::resolveDomain("google.com");
  printf("DNS found: %i.%i.%i.%i\n", ip4_addr1(&result.u_addr.ip4),
         ip4_addr2(&result.u_addr.ip4), ip4_addr3(&result.u_addr.ip4),
         ip4_addr4(&result.u_addr.ip4));

  while (true) {
  }
}

void app_main() {
  SmartConfig.begin(60000);
  xTaskCreate(&task, "task", 8192, NULL, 5, NULL);
}
