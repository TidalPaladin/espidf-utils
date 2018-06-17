#include "SmartConfig.h"

const char *SmartConfigStatic::TAG = "SC";
EventGroupHandle_t SmartConfigStatic::wifi_event_group;

esp_err_t SmartConfigStatic::begin() {
  initAdapter();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  // Initialize adapter in client mode, store network in flash
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);
  esp_wifi_set_auto_connect(true);
  esp_wifi_start();
  return ESP_OK;
}

esp_err_t SmartConfigStatic::begin(uint32_t timeout_ms) {
  esp_err_t err = begin();
  return err == ESP_OK ? blockForSmartConfig(timeout_ms) : err;
}

esp_err_t SmartConfigStatic::blockForSmartConfig(uint32_t timeout_ms) {
  TickType_t xTicksToWait = Delay.millisecondsToTicks(timeout_ms);
  EventBits_t uxBits = xEventGroupWaitBits(
      wifi_event_group, ESPTOUCH_CONNECTED_BIT | ESPTOUCH_DONE_BIT, pdTRUE,
      pdFALSE, xTicksToWait);

  if ((uxBits & ESPTOUCH_CONNECTED_BIT) != 0) {
    ESP_LOGI(TAG, "Connection established, leaving begin()");
    ip();
    return ESP_OK;
  } else {
    ESP_LOGI(TAG, "Connection timeout, leaving begin()");
    return ESP_ERR_TIMEOUT;
  }
}

esp_err_t SmartConfigStatic::initAdapter() {
  ESP_LOGI(TAG, "Initializing adapter");
  tcpip_adapter_init();
  nvs_flash_init();
  wifi_event_group = xEventGroupCreate();
  esp_event_loop_init(eventHandler, NULL);
  return ESP_OK;
}

esp_err_t SmartConfigStatic::forceSmartConfig() {
  esp_err_t err;
  err = esp_wifi_disconnect();
  xTaskCreate(smartConfigTask, "smart_config", 4096, NULL, 3, NULL);
  return err;
}

esp_err_t SmartConfigStatic::forceSmartConfig(uint32_t timeout_ms) {
  forceSmartConfig();
  return blockForSmartConfig(timeout_ms);
}

esp_err_t SmartConfigStatic::eventHandler(void *ctx, system_event_t *event) {
  // Here we handle WiFi events not related to smart config
  switch (event->event_id) {
    // On wifi start, launch smart config task
    case SYSTEM_EVENT_STA_START:
      bool autoconnect;
      esp_wifi_get_auto_connect(&autoconnect);
      if (!autoconnect || connect() != ESP_OK)
        xTaskCreate(smartConfigTask, "smart_config", 4096, NULL, 3, NULL);
      else {
        xEventGroupSetBits(wifi_event_group, ESPTOUCH_CONNECTED_BIT);
      }
      break;

    case SYSTEM_EVENT_STA_GOT_IP: {
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
      ip4_addr_t ipv4 = ip();
      ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ipv4));
      xEventGroupSetBits(wifi_event_group, ESPTOUCH_CONNECTED_BIT);
      break;
    }
    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      xEventGroupClearBits(wifi_event_group, ESPTOUCH_CONNECTED_BIT);
      break;
    default:
      break;
  }
  return ESP_OK;
}

void SmartConfigStatic::smartConfigCallback(smartconfig_status_t status,
                                            void *pdata) {
  wifi_config_t *wifi_config;
  switch (status) {
    case SC_STATUS_WAIT:
      ESP_LOGI(TAG, "SC_STATUS_WAIT");
      break;
    case SC_STATUS_FIND_CHANNEL:
      ESP_LOGI(TAG, "SC_STATUS_FINDING_CHANNEL");
      break;
    case SC_STATUS_GETTING_SSID_PSWD:
      ESP_LOGI(TAG, "SC_STATUS_GETTING_SSID_PSWD");
      break;
    case SC_STATUS_LINK:
      ESP_LOGI(TAG, "SC_STATUS_LINK");
      wifi_config = (wifi_config_t *)pdata;
      connect(wifi_config);
      break;
    case SC_STATUS_LINK_OVER:
      ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
      if (pdata != NULL) {
        uint8_t phone_ip[4] = {0};
        memcpy(phone_ip, (uint8_t *)pdata, 4);
        ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
                 phone_ip[2], phone_ip[3]);
      }
      xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
      break;
    default:
      break;
  }
}

void SmartConfigStatic::smartConfigTask(void *parm) {
  ESP_LOGI(TAG, "Starting SC task...\n");
  EventBits_t uxBits;
  esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
  esp_smartconfig_start(smartConfigCallback);
  while (1) {
    // Delay while SC connection is made
    uxBits = xEventGroupWaitBits(wifi_event_group,
                                 ESPTOUCH_CONNECTED_BIT | ESPTOUCH_DONE_BIT,
                                 true, false, portMAX_DELAY);

    // When SC wait is over, report results
    if (uxBits & ESPTOUCH_CONNECTED_BIT) {
      ESP_LOGI(TAG, "WiFi Connected to ap");
    }
    if (uxBits & ESPTOUCH_DONE_BIT) {
      ESP_LOGI(TAG, "smartconfig over");
      esp_smartconfig_stop();
      vTaskDelete(NULL);
    }
  }
}

esp_err_t SmartConfigStatic::connect(wifi_config_t *config) {
  if (config == nullptr) {
    ESP_LOGI(TAG, "Got null pointer, trying using saved settings");
    esp_wifi_connect();
    return ESP_OK;
  }

  // Check good wifi config
  if (!isValidSSID(config)) {
    ESP_LOGE(TAG, "Got bad ssid \"%s\"", config->sta.ssid);
    return ESP_ERR_INVALID_ARG;
  } else if (!isValidPSK(config)) {
    ESP_LOGE(TAG, "Got bad psk \"%s\"", config->sta.password);
    return ESP_ERR_INVALID_ARG;
  }

  ESP_LOGI(TAG, "SSID:%s", config->sta.ssid);
  ESP_LOGI(TAG, "PASSWORD:%s", config->sta.password);

  // Disconnect, set config, and connect
  esp_wifi_disconnect();
  esp_wifi_set_config(ESP_IF_WIFI_STA, config);
  esp_wifi_connect();
  return ESP_OK;
}

bool SmartConfigStatic::isValidSSID(wifi_config_t *config) {
  return config->sta.ssid[0] != 0;
}
bool SmartConfigStatic::isValidPSK(wifi_config_t *config) {
  return config->sta.password[0] != 0;
}

ip4_addr_t SmartConfigStatic::ip() {
  tcpip_adapter_ip_info_t ip;
  memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
  if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip) == 0) {
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ip.ip));
    ESP_LOGI(TAG, "MASK:" IPSTR, IP2STR(&ip.netmask));
    ESP_LOGI(TAG, "GW:" IPSTR, IP2STR(&ip.gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
  }
  return ip.ip;
}