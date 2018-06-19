#include "DNS.h"

namespace DNS {
const char *TAG = "DNS";
EventGroupHandle_t dns_event_group;
}  // namespace DNS

esp_err_t DNS::resolve(const char *url, ip_addr_t *dest) {
  ESP_LOGI(TAG, "Resolve URL: %s", url);
  dns_event_group = xEventGroupCreate();

  /* Start DNS query, block until completion */
  esp_err_t result = dns_gethostbyname(url, dest, dns_found_cb, NULL);
  xEventGroupWaitBits(dns_event_group, DNS_DONE_BIT, pdTRUE, pdFALSE,
                      portMAX_DELAY);

  return result;
}

void DNS::dns_found_cb(const char *name, const ip_addr_t *ipaddr,
                       void *callback_arg) {
  ESP_LOGI(TAG, "DNS found:" IPSTR, IP2STR(&ipaddr->u_addr.ip4));
  xEventGroupSetBits(dns_event_group, DNS_DONE_BIT);
}