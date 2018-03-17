#ifndef __NVS_HELPER_H__
#define __NVS_HELPER_H__

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
#include "nvs.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "Delay/Delay.h"

class NVSStatic {

  private:
	static const char *TAG;
	static nvs_handle my_handle;

  public:
	/**
	 * @brief This must be called to start NVS
	 *
	 * post: NVS initialized in read/write state
	 *
	 * @return esp_err_t
	 */
	static esp_err_t begin();

	/**
	 * @brief This must be called to close the NVS dialog
	 *
	 * post: NVS no longer readable/writable, and cleanly closed
	 *
	 * @return esp_err_t
	 */
	static esp_err_t end();

	/**
	 * @brief Reads a value fron NVS using a key
	 *
	 * @param key   The key to find the associated value for
	 * @param dest  A reference to the variable where the value should be put
	 *
	 * @return
	 *  - ESP_OK                    The read was successful
	 *  - ESP_ERR_NVS_NOT_FOUND     The given key was not found
	 */
	static esp_err_t read_i8(const char *key, int8_t &dest);
	static esp_err_t read_i16(const char *key, int16_t &dest);
	static esp_err_t read_i32(const char *key, int32_t &dest);
	static esp_err_t read_u8(const char *key, uint8_t &dest);
	static esp_err_t read_u16(const char *key, uint16_t &dest);
	static esp_err_t read_u32(const char *key, uint32_t &dest);
	static esp_err_t read_str(const char *key, char *dest, size_t dest_len);
	static esp_err_t read_blob(const char *key, void *dest, size_t dest_len);

	/**
	 * @brief Writes a value to NVS for the given key
	 *
	 * @param key   The key to write the associated value for
	 * @param dest  The variable to write
	 *
	 * @return
	 *  - ESP_OK                    The write was successful
	 */
	static esp_err_t write_i8(const char *key, int8_t &data);
	static esp_err_t write_i16(const char *key, int16_t &data);
	static esp_err_t write_i32(const char *key, int32_t &data);
	static esp_err_t write_u8(const char *key, uint8_t &data);
	static esp_err_t write_u16(const char *key, uint16_t &data);
	static esp_err_t write_u32(const char *key, uint32_t &data);
	static esp_err_t write_str(const char *key, const char *data);
	static esp_err_t write_blob(const char *key, void *src, size_t len);

	static esp_err_t erase_key(const char *key);

	static esp_err_t erase_all();

  private:
	/**
	 * @brief Call after write() to commit the change to NVS
	 *
	 * @return esp_err_t
	 */
	static esp_err_t nvsCommit();

	static esp_err_t checkReadResult(esp_err_t result) {
		if (result != ESP_OK) {
			ESP_LOGE(TAG, "Error (%i) reading from NVS", result);
			errToName(result);
		}
		return result;
	}

	static esp_err_t checkWriteResult(esp_err_t result) {
		if (result != ESP_OK) {
			ESP_LOGE(TAG, "Error (%i) writing to NVS", result);
			errToName(result);
		} else {
			nvsCommit();
		}
		return result;
	}

	static void errToName(const esp_err_t &err) {
		switch (err) {
		case ESP_ERR_NVS_NOT_FOUND:
			ESP_LOGE(TAG, "ESP_ERR_NVS_NOT_FOUND");
			break;
		case ESP_ERR_NVS_KEY_TOO_LONG:
			ESP_LOGE(TAG, "ESP_ERR_NVS_KEY_TOO_LONG");
			break;
		case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
			ESP_LOGE(TAG, "ESP_ERR_NVS_NOT_ENOUGH_SPACE");
			break;
		case ESP_ERR_NVS_READ_ONLY:
			ESP_LOGE(TAG, "ESP_ERR_NVS_READ_ONLY");
			break;
		case ESP_ERR_NVS_NOT_INITIALIZED:
			ESP_LOGE(TAG, "ESP_ERR_NVS_NOT_INITIALIZED");
			break;
		case ESP_ERR_NVS_VALUE_TOO_LONG:
			ESP_LOGE(TAG, "ESP_ERR_NVS_VALUE_TOO_LONG");
			break;
		case ESP_ERR_NVS_INVALID_HANDLE:
			ESP_LOGE(TAG, "ESP_ERR_NVS_INVALID_HANDLE");
			break;
		default:
			break;
		}
	}
};

extern NVSStatic NVS;

#endif