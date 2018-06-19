/**
 * Library designed to provide easy access to ESP smart config features.
 * Using the mobile device app, connecting an ESP32 to wifi can be done easily
 *
 * @author Scott Chase Waggener <tidalpaladin@gmail.com>
 * @date 6/16/18
 *
 * FLOW DESCRIPTION:
 *
 * 1) begin() is called
 */

#ifndef __ESP_SMART_CONFIG_HELPER_H__
#define __ESP_SMART_CONFIG_HELPER_H__

#include <stdlib.h>
#include <string.h>

#include "Delay/Delay.h"
#include "EasyWifi.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#define ESPTOUCH_CONNECTED_BIT BIT0
#define ESPTOUCH_DONE_BIT BIT1

/* The name of the key for the NVS key-value pair? */
#define SC_NVS_KEY "SC_KEY"

/* SC error defines */
#define ESP_ERR_SC_OK ESP_OK /*!< No error */
#define ESP_ERR_SC_NOT_SET \
  (ESP_ERR_WIFI_BASE + 1) /*!< Field (ssid/psk) not set */

namespace SmartConfig {
extern EventGroupHandle_t wifi_event_group;
extern const char *TAG;

/**
 * @brief Starts the SC task, waiting for smartphone connection
 *
 * @param timeout_s  The time in seconds after which the SC task will be
 * terminated
 *
 * @return ESP_OK
 */
esp_err_t sc_start(uint32_t *timeout_s);

/**
 * @brief Callback handler for SmartConfig events
 *
 * @param status    The event that triggered the callback
 * @param pdata     Data
 *
 */
void sc_callback(smartconfig_status_t status, void *pdata);

/**
 * @brief Creates a task to listen for ESPTOUCH connections
 *
 * post: New task created, listening for SSID / passphrase data from
 * smartphone
 *
 */
void sc_task(void *parm);

/**
 * @brief Task that handles connecting to wifi
 *
 */
void wifi_conn_task(void *parm);

}  // namespace SmartConfig

#endif