#include "Update.h"

namespace Update {
const char *const TAG = "OTA";
}

/* Checks that the update partition is valid and ready to go */
esp_err_t Update::begin() {
  ESP_LOGI(TAG, "Beginning OTA update");

  const esp_partition_t *const _pUpdatePartition =
      esp_ota_get_next_update_partition(NULL);

  // Make sure partition is available
  if (_pUpdatePartition == nullptr) {
    ESP_LOGE(TAG, "Unable to get next update partition");
    return ESP_ERR_OTA_BASE;
  }

  /* Check partitions */
  ESP_ERROR_CHECK(configured_part_is_running_part());

  ESP_ERR_CHECK(
      esp_ota_begin(_pUpdatePartition, OTA_SIZE_UNKNOWN, &_updateHandle));
  return ESP_OK;
}

esp_err_t Update::download_update() {
  esp_http_client_config_t config = {
      .url = "https://www.howsmyssl.com",
      .cert_pem = howsmyssl_com_root_cert_pem_start,
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  }

  esp_http_client_cleanup(client);
}

esp_err_t Update::write_update(uint8_t *data, size_t size) {
  esp_err_t ret = ESP_OK;

  UP_ERR_CHECK(_began, ESP_ERR_OTA_BASE, "Cannot call write() before begin()");

  ret = esp_ota_write(_updateHandle, data, size);
  UP_ERR_CHECK(ret, ret, "Could not write OTA data");

  return ESP_OK;
}

esp_err_t Update::end() {
  _reset();

  return ESP_OK;
}

/**
 *
 *  Privates
 *
 */

static esp_err_t _http_event_handle(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
      printf("%.*s", evt->data_len, (char *)evt->data);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      if (!esp_http_client_is_chunked_response(evt->client)) {
        printf("%.*s", evt->data_len, (char *)evt->data);
      }

      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
      break;
  }
  return ESP_OK;
}

bool Update::_partition_check() {
  const esp_partition_t const *_pRunning = esp_ota_get_running_partition();
  const esp_partition_t const *_pConfigured = esp_ota_get_boot_partition();

  if (_pConfigured != _pRunning) {
    ESP_LOGW(TAG,
             "Configured OTA boot partition at offset 0x%08x, but running from "
             "offset 0x%08x",
             _pConfigured->address, _pRunning->address);
    ESP_LOGW(TAG,
             "(This can happen if either the OTA boot data or preferred boot "
             "image become corrupted somehow.)");
    return ESP_ERR_OTA_PARTITION_CONFLICT;
  }
  ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
           _pRunning->type, _pRunning->subtype, _pRunning->address);

  return ESP_OK;
}
