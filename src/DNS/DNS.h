#ifndef __DNS_H__
#define __DNS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

#define DNS_DONE_BIT 1

namespace DNS {

extern const char *TAG;
extern EventGroupHandle_t dns_event_group;

/*
 * @brief Resolve the IP address for a given domain using DNS
 *
 * @param url   The target domain's URL
 *
 * @return The IP address for 'url'
 */
esp_err_t resolve(const char *url, ip_addr_t *dest);

void dns_found_cb(const char *name, const ip_addr_t *ipaddr,
                  void *callback_arg);

}  // namespace DNS
#endif