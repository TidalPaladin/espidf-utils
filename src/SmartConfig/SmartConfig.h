/**
 * Library designed to provide easy access to ESP smart config features.
 * Using the mobile device app, connecting an ESP32 to wifi can be done easily
 *
 * @author Scott Chase Waggener <tidalpaladin@gmail.com>
 * @date 6/16/18
 */

#ifndef __ESP_SMART_CONFIG_HELPER_H__
#define __ESP_SMART_CONFIG_HELPER_H__

#include <stdlib.h>
#include <string.h>
#include "Delay/Delay.h"
#include "NVS/NVS.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#define ESPTOUCH_CONNECTED_BIT BIT0
#define ESPTOUCH_DONE_BIT BIT1

#define SC_NVS_KEY "SC_KEY"

class SmartConfigStatic {
 private:
  static const char *TAG;
  static EventGroupHandle_t wifi_event_group;

 public:
  /**
   * @brief Begins the SmartConfig connection service without blocking
   *
   *
   * post: ESPTOUCH started and listening in separate task
   *
   * @return ESP_OK
   */
  static esp_err_t begin();

  /**
   * @brief 	Begins the SmartConfig connection service, blocking until
   * SmartConfig completes or a timeout is reached
   *
   * @param timeout_ms	Maximum time to wait for ESPTOUCH in milliseconds
   *
   * post: Waiting for ESPTOUCH SSID and passphrase data from smartphone app
   *
   * @return
   * 	- ESP_OK 			ESPTOUCH was successful
   *  - ESP_ERR_TIMEOUT 	No connection was established before timeout
   */
  static esp_err_t begin(uint32_t timeout_ms);

  /**
   * @brief Initializes the tcpip adapter
   *
   *
   * @return esp_err_t
   */
  static esp_err_t initAdapter();

  /**
   * @brief Connect using the supplied wifi config
   *
   * @param config	Pointer to the wifi config used to connect
   *
   * @return error code
   */
  static esp_err_t connect(wifi_config_t *config = nullptr);

  static esp_err_t forceSmartConfig();
  static esp_err_t forceSmartConfig(uint32_t timeout_ms);

  static ip4_addr_t ip();

 private:
  /**
   * @brief Callback handler for SmartConfig events
   *
   * @param status    The event that triggered the callback
   * @param pdata     Data
   *
   */
  static void smartConfigCallback(smartconfig_status_t status, void *pdata);

  /**
   * @brief Creates a task to listen for ESPTOUCH connections
   *
   * post: New task created, listening for SSID / passphrase data from
   * smartphone
   *
   */
  static void smartConfigTask(void *parm);

  /**
   * @brief Event handler for generic network events not related to ESPTOUCH
   *
   * @param ctx
   *
   * @param event The network event that triggered the callback
   *
   * @return esp_err_t
   */
  static esp_err_t eventHandler(void *ctx, system_event_t *event);

  SmartConfigStatic() {}

  static bool isValidSSID(wifi_config_t *config);
  static bool isValidPSK(wifi_config_t *config);

  static esp_err_t blockForSmartConfig(uint32_t timeout_ms);
};

extern SmartConfigStatic SmartConfig;

#endif