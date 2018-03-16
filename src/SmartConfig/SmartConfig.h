#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"

#define ESPTOUCH_CONNECTED_BIT BIT0
#define ESPTOUCH_DONE_BIT BIT1

#ifndef __ESP_SMART_CONFIG_HELPER_H__
#define __ESP_SMART_CONFIG_HELPER_H__

class SmartConfigStatic {

  private:
	static const char *TAG;
	static EventGroupHandle_t wifi_event_group;

  public:
	/**
	 * @brief Begins the SmartConfig connection service
	 *
	 * post: Waiting for ESPTOUCH SSID and passphrase data from smartphone app
	 *
	 * @return esp_err_t
	 */
	static esp_err_t begin();

	/**
	 * @brief Initializes the tcpip adapter
	 *
	 *
	 * @return esp_err_t
	 */
	static esp_err_t initAdapter();

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
};

extern SmartConfigStatic SmartConfig;

#endif