
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

#ifndef __ESP_EASY_WIFI_H__
#define __ESP_EASY_WIFI_H__

#include <stdlib.h>
#include <string.h>
#include "Delay/Delay.h"
#include "NVS/NVS.h"
#include "esp_err.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#define EW_DEFAULT_WAIT_MS 10000
#define ESP_WIFI_CONN_BIT 1

/* The name of the key for the NVS key-value pair? */
#define SC_NVS_KEY "SC_KEY"

/* SC error defines */
#define ESP_ERR_SC_OK ESP_OK /*!< No error */
#define ESP_ERR_SC_NOT_SET \
  (ESP_ERR_WIFI_BASE + 1) /*!< Field (ssid/psk) not set */

namespace EasyWifi {

extern const char *TAG;
extern EventGroupHandle_t wifi_event_group;

/**
 * @brief Initializes the tcpip adapter and nvs
 *
 *
 * @return esp_err_t
 */
esp_err_t init_hardware();

/**
 * @brief Initializes the wifi stack
 *
 * @return esp_err_t
 */
esp_err_t init_software();

/**
 * @brief Attempt wifi connection
 *
 * @param wait_ms   How long in ms to wait before falling back to SC
 *
 */
esp_err_t connect();

/**
 * @brief Attempt wifi connection using the supplied config
 *
 * @param config    Pointer to the config containing SSID and PSK
 * @param wait_ms   How long in ms to wait before falling back to SC
 *
 */
esp_err_t connect(wifi_config_t *config);

/**
 * @brief Blocks the current task for a given time until wifi connects or the
 * timeout is reached. Don't call this if you want the connection to proceed
 * asynchronously.
 *
 * @param time_s    Seconds after which the wifi connect attempt will abort
 *
 */
void wait_for_wifi(uint32_t time_s);

esp_err_t enable_fallback(uint32_t timeout_s);

/**
 * @brief Gets the current IPv4 address and logs the full address to log level
 * INFO
 *
 * @return The current IPv4 address
 */
ip4_addr_t getIP();

/**
 * @brief Gets a data structure containing information about the tcpip adapter
 * state, including IP address, subnet mask, and default gateway
 *
 * @return tcpip_adater_ip_info_t
 */
tcpip_adapter_ip_info_t getConnectionInfo();

/**
 * @brief Event handler for generic network events not related to ESPTOUCH
 *
 * @param ctx
 *
 * @param event The network event that triggered the callback
 *
 * @return esp_err_t
 */
esp_err_t wifi_event_handler(void *ctx, system_event_t *event);

/**
 * Checks if the supplied config has a valid SSID and PSK
 *
 * @param config    Pointer to a wifi config
 *
 * @return is_valid_ssid(config) && is_valid_psk(config)
 */
bool is_valid_config(wifi_config_t *config);

/**
 * Checks if the supplied config has a valid SSID
 *
 * @param config    Pointer to a wifi config
 *
 * @return True if something has been
 */
bool is_valid_ssid(wifi_config_t *config);

/**
 * Checks if the supplied config has a valid passphrase
 *
 * @param config    Pointer to a wifi config
 *
 * @return true if PSK has between 8 and 63 alphanumeric characters
 */
bool is_valid_psk(wifi_config_t *config);

bool is_connected();

/**
 * Block code execution while waiting to receive wifi info.
 *
 */
esp_err_t blockForEasyWifi(uint32_t timeout_ms);
}  // namespace EasyWifi

#endif