#ifndef __IDF_MQTT_H__
#define __IDF_MQTT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "apps/sntp/sntp.h"
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

#define ESP_WEMO_DEV_NAME "WemoESP"
#define ESP_WEMO_TAG "wemo"

typedef struct {
  char *name;

} wemo_config_t;

typedef void (*wemo_callback_t)(void *);

esp_err_t esp_wemo_init(wemo_config_t *config);

static esp_err_t esp_wemo_send_header();

#endif