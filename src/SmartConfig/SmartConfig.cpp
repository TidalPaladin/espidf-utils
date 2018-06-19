#include "SmartConfig.h"

namespace SmartConfig {
const char *TAG = "SC";
EventGroupHandle_t sc_event_group;
}  // namespace SmartConfig

esp_err_t SmartConfig::sc_start(uint32_t *timeout_s) {
  /* Clear bits and launch task */
  sc_event_group = xEventGroupCreate();
  void *parm = static_cast<void *>(timeout_s);
  xTaskCreate(sc_task, "smart_config", 4096, parm, 3, NULL);
  return ESP_OK;
}

/* Handles SC events */
void SmartConfig::sc_callback(smartconfig_status_t status, void *pdata) {
  wifi_config_t *wifi_config;
  switch (status) {
    /* Link start */
    case SC_STATUS_LINK:
      ESP_LOGI(TAG, "SC_STATUS_LINK");
      wifi_config = (wifi_config_t *)pdata;
      EasyWifi::connect(wifi_config);
      break;

    case SC_STATUS_LINK_OVER:
      ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
      xEventGroupSetBits(sc_event_group, ESPTOUCH_DONE_BIT);
      break;

    /* Below events are not that important */
    case SC_STATUS_WAIT:
      ESP_LOGI(TAG, "SC_STATUS_WAIT");
      break;
    case SC_STATUS_FIND_CHANNEL:
      ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
      break;
    case SC_STATUS_GETTING_SSID_PSWD:
      ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
      break;
    default:
      break;
  }
}

void SmartConfig::sc_task(void *parm) {
  ESP_LOGI(TAG, "Starting SC task...\n");

  /* uint32_t timeout_s passed as param */
  uint32_t *timeout_s = static_cast<uint32_t *>(parm);
  TickType_t timeout_ticks = Delay.millisecondsToTicks(*timeout_s * 1000);

  EventBits_t uxBits;
  esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
  esp_smartconfig_start(sc_callback);

  /* Delay until completion or timeout is reached */
  uxBits = xEventGroupWaitBits(sc_event_group,
                               ESPTOUCH_CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                               pdTRUE, pdFALSE, timeout_ticks);

  // When SC wait is over, report results
  if (uxBits & ESPTOUCH_CONNECTED_BIT) {
    ESP_LOGI(TAG, "ESPTOUCH_CONNECTED_BIT set");
  } else {
    ESP_LOGI(TAG, "smart config failed");
  }
  if (uxBits & ESPTOUCH_DONE_BIT) {
    ESP_LOGI(TAG, "ESPTOUCH_DONE_BIT set");
  }

  esp_smartconfig_stop();
  ESP_LOGI(TAG, "Leaving sc_task");
  vTaskDelete(NULL);
}
