#ifndef __IDF_MQTT_H__
#define __IDF_MQTT_H__

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

typedef struct {
    char name[64];
    char state[32];
    bool auto_publish;
    uint32_t publish_period_ms;
} mqtt_topic_t;

typedef void(mqtt_callback_t)(mqtt_topic_t *);

esp_err_t esp_mqtt_enable_topic(mqtt_topic_t *topic);

esp_err_t esp_mqtt_disable_topic(mqtt_topic_t *topic);

#endif