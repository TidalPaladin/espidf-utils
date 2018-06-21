/*
  Update.h
  Scott Chase Waggener  tidalpaladin@gmail.com
  10/10/2017

  Implemented using ESP-IDF

  Deisgned to handle writing firmware updates to the ESP32

*/

#ifndef UPDATE_H
#define UPDATE_H

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>  // WRITE
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Update {

/**
 * @brief Initializes the update process
 *
 * @return esp_err_t
 */
static esp_err_t begin();

/**
 * @brief Fetches a firmware.bin file from a HTTPS server
 *
 * @param url The location of the firmware file on a HTTPS server
 *
 * @return esp_err_t
 */

static esp_err_t download_update();

/**
 * @brief Writes supplied data to the update partition
 *
 * @param data A uint8_t array to write
 * @param size The size of the incoming data block
 *
 * @return esp_err_t
 */
static esp_err_t write_update(uint8_t *data, size_t size);

/**
 * #brief Ends the update, checking for errors
 *
 * #return esp_err_t
 */
static esp_err_t end();

static esp_err_t _http_event_handle(esp_http_client_event_t *evt);

/* set with esp_ota_begin(), free with esp_ota_end() */
static esp_ota_handle_t _updateHandle;

/* Partition pointers for running and configured */
static const esp_partition_t const *_pConfigured;
static const esp_partition_t const *_pRunning;

static const esp_partition_t *_pUpdatePartition;

static const char *const TAG;

/**
 * @brief Verifies that the currently running partition matches
 * the configured partition.
 *
 * Configured and running partitions will mismatch if the configured
 * partition is bad and the ESP falls back to a different partition.
 *
 * @return
 *  - true if the running and configured partitions are the same
 *  - false otherwise
 */
static esp_err_t configured_part_is_running_part();

/**
 * @brief Cleans up the updating process. Call this if an error
 * is encountered or the update has completed successfully
 *
 * @return esp_err_t
 */
static esp_err_t _reset();

static bool connectToHTTPS(const char *hostname);
};  // namespace Update

#endif
