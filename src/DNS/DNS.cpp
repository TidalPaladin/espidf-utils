#include "DNS.h"

namespace DNS {
bool _waitingDnsResolve;
ip_addr_t _resolvedDomain;
}  // namespace DNS

ip_addr_t DNS::resolveDomain(const char *url) {
  ESP_LOGI("DNS", "Resolve URL: %s\n", url);
  _waitingDnsResolve = true;
  ip_addr_t ip;

  /* Start DNS query, block until completion */

  dns_gethostbyname(url, &ip, dns_found_cb, NULL);
  while (_waitingDnsResolve) {
  }

  return _resolvedDomain;
}

void DNS::dns_found_cb(const char *name, const ip_addr_t *ipaddr,
                       void *callback_arg) {
  _resolvedDomain = *ipaddr;
  _waitingDnsResolve = false;
}