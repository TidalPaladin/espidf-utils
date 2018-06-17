#ifndef __DNS_H__
#define __DNS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/dns.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"

namespace DNS {

extern bool _waitingDnsResolve;
extern ip_addr_t _resolvedDomain;

/**
 * @brief Resolve the IP address for a given domain using DNS
 *
 * @param url   The target domain's URL
 *
 * @return The IP address for 'url'
 */
ip_addr_t resolveDomain(const char *url);

static void _dnsResolveTask(const char *url);
void dns_found_cb(const char *name, const ip_addr_t *ipaddr,
                  void *callback_arg);

}  // namespace DNS

#endif