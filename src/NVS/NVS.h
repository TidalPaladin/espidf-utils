#ifndef __NVS_HELPER_H__
#define __NVS_HELPER_H__

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

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
	template <typename T> static esp_err_t read(const char *key, T &dest) {
		esp_err_t ret = nvs_get_blob(my_handle, key, dest, sizeof(T));
		return checkReadResult(ret);
	}

	/**
	 * @brief Reads a string from NVS
	 *
	 * @param key   The key to find the associated string for
	 * @param dest  A reference to the buffer where the string should be put
	 * @param len  	The length of the buffer
	 *
	 * @return
	 *  - ESP_OK                    The read was successful
	 *  - ESP_ERR_NVS_NOT_FOUND     The given key was not found
	 */
	static esp_err_t read_str(const char *key, char *dest, size_t &len);

	/**
	 * @brief Writes a value to NVS for the given key
	 *
	 * @param key   The key to write the associated value for
	 * @param src  The variable to write
	 *
	 * @return
	 *  - ESP_OK                    The write was successful
	 */
	template <typename T> static esp_err_t write(const char *key, T &src) {
		esp_err_t ret = nvs_set_blob(my_handle, key, src, sizeof(T));
		return checkWriteResult(ret);
	}

	/**
	 * @brief Write a string to NVS
	 *
	 * @param key   The key to find the associated string for
	 * @param src  	The string to write
	 *
	 * @return
	 *  - ESP_OK                    The write was successful
	 */
	static esp_err_t write_str(const char *key, const char *src);

	/**
	 * @brief Erases the given key from NVS
	 *
	 * @param key	The key to erase
	 *
	 * @return
	 *  - ESP_OK		The erase was successful
	 */
	static esp_err_t erase_key(const char *key);

	/**
	 * @brief Erases all NVS keys set by this library
	 *
	 * @return
	 *  - ESP_OK		The erase was successful
	 */
	static esp_err_t erase_all();

  private:
	/**
	 * @brief Call after write() to commit the change to NVS
	 *
	 * @return esp_err_t
	 */
	static esp_err_t nvsCommit();

	/**
	 * @brief Prints error messages for read methods
	 *
	 * Call this after each read
	 *
	 * @return result
	 */
	static esp_err_t checkReadResult(esp_err_t result);

	/**
	 * @brief Prints error messages for write methods and commits the set value
	 *
	 * Call this after each write
	 *
	 * @return result
	 */
	static esp_err_t checkWriteResult(esp_err_t result);

	/**
	 * @brief 	Converts an esp_err_t to its enum text name and prints the
	 * resulting error
	 *
	 * post: LOGE message printed if err != ESP_OK
	 */
	static void errToName(const esp_err_t &err);
};

extern NVSStatic NVS;

#endif