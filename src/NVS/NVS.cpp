#include "NVS.h"

NVSStatic NVS;
nvs_handle NVSStatic::my_handle;
const char *NVSStatic::TAG = "NVS";

esp_err_t NVSStatic::begin(const char *name_space) {
  ESP_LOGI(TAG, "Initializing NVS");
  esp_err_t result = nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    result = nvs_flash_init();
  }

  if (result != ESP_OK) {
    ESP_LOGW(TAG, "Error (%d) while initializing NVS", result);
    return result;
  }

  ESP_LOGI(TAG, "Opening NVS");
  result = nvs_open(name_space, NVS_READWRITE, &my_handle);
  if (result != ESP_OK) {
    ESP_LOGW(TAG, "Error (%d) while opening NVS", result);
  } else {
    ESP_LOGI(TAG, "NVS opened");
  }
  return result;
}

esp_err_t NVSStatic::read(const char *key, int8_t &dest) {
  esp_err_t ret = nvs_get_i8(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, int16_t &dest) {
  esp_err_t ret = nvs_get_i16(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, int32_t &dest) {
  esp_err_t ret = nvs_get_i32(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, uint8_t &dest) {
  esp_err_t ret = nvs_get_u8(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, uint16_t &dest) {
  esp_err_t ret = nvs_get_u16(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, uint32_t &dest) {
  esp_err_t ret = nvs_get_u32(my_handle, key, &dest);
  return checkReadResult(ret, key);
}
esp_err_t NVSStatic::read(const char *key, char *dest) {
  esp_err_t ret = nvs_get_str(my_handle, key, dest, nullptr);
  return checkReadResult(ret, key);
}

esp_err_t NVSStatic::write(const char *key, int8_t &data) {
  esp_err_t ret = nvs_set_i8(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, int16_t &data) {
  esp_err_t ret = nvs_set_i16(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, int32_t &data) {
  esp_err_t ret = nvs_set_i32(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, uint8_t &data) {
  esp_err_t ret = nvs_set_u8(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, uint16_t &data) {
  esp_err_t ret = nvs_set_u16(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, uint32_t &data) {
  esp_err_t ret = nvs_set_u32(my_handle, key, data);
  return checkWriteResult(ret, key);
}
esp_err_t NVSStatic::write(const char *key, const char *data) {
  esp_err_t ret = nvs_set_str(my_handle, key, data);
  return checkWriteResult(ret, key);
}

esp_err_t NVSStatic::end() {
  nvs_close(my_handle);
  ESP_LOGI(TAG, "NVS closed");
  return ESP_OK;
}

esp_err_t NVSStatic::nvsCommit() {
  ESP_LOGV(TAG, "Commit NVS changes");
  esp_err_t err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%i) in NVS commit", err);
  }
  return err;
}

esp_err_t NVSStatic::erase_key(const char *key) {
  esp_err_t result = nvs_erase_key(my_handle, key);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error (%i) erasing key \"%s\"", result, key);
  }
  return result;
}

esp_err_t NVSStatic::erase_all() {
  esp_err_t result = nvs_erase_all(my_handle);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error (%i) erasing all keys", result);
  }
  return result;
}

esp_err_t NVSStatic::checkReadResult(esp_err_t result, const char *key) {
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error (%i) reading key \"%s\" from NVS", result, key);
    errToName(result);
  } else {
    ESP_LOGI(TAG, "Read key \"%s\" from NVS", key);
  }
  return result;
}

esp_err_t NVSStatic::checkWriteResult(esp_err_t result, const char *key) {
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Error (%i) writing key \"%s\" to NVS", result, key);
    errToName(result);
  } else {
    nvsCommit();
    ESP_LOGI(TAG, "Wrote key \"%s\" to NVS", key);
  }
  return result;
}

void NVSStatic::errToName(const esp_err_t &err) {
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