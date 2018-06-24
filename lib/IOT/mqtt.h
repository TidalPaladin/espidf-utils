#ifndef __IDF_MQTT_H__
#define __IDF_MQTT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aws_iot_config.h"
#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"
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
  ip_addr_t broker;
  char *client_id;
} mqtt_config_t;

typedef struct {
  char *name;
  char *state;
  bool auto_publish;
  uint32_t publish_period_ms;
  mqtt_callback_t *callback;
} mqtt_topic_t;

static mqtt_config_t *active_config;

typedef void(mqtt_callback_t)(mqtt_topic_t *);

esp_err_t esp_mqtt_set_config(mqtt_config_t *config);

esp_err_t esp_mqtt_begin();

esp_err_t esp_mqtt_end();

esp_err_t esp_mqtt_enable_topic(mqtt_topic_t *topic);

esp_err_t esp_mqtt_disable_topic(mqtt_topic_t *topic);

#endif