#include "EasyWifi.h"

namespace EasyWifi {
const char *TAG = "EW";
EventGroupHandle_t wifi_event_group;
}  // namespace EasyWifi

esp_err_t EasyWifi::connect() { return connect(nullptr); }

esp_err_t EasyWifi::connect(wifi_config_t *config) {
  if (is_connected()) {
    ESP_LOGE(TAG, "Not connecting because wifi is already connected");
    ESP_LOGI(TAG, "Call esp_wifi_disconnect before connecting");
    return ESP_ERR_WIFI_STATE;
  }

  /* If passed a null config, try autoconnect */
  if (config == nullptr) {
    ESP_LOGI(TAG, "Got null pointer, attempting autoconnect");
  }
  /* Else use supplied config */
  else {
    if (!is_valid_ssid(config)) {
      ESP_LOGE(TAG, "Got bad ssid \"%s\"", config->sta.ssid);
      return ESP_ERR_INVALID_ARG;
    } else if (!is_valid_psk(config)) {
      ESP_LOGE(TAG, "Got bad psk \"%s\"", config->sta.password);
      return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "SSID:%s", config->sta.ssid);
    ESP_LOGI(TAG, "PASSWORD:%s", config->sta.password);
    esp_wifi_set_config(ESP_IF_WIFI_STA, config);
  }

  /* No error check here, dont want to abort on accidental fail */
  return esp_wifi_connect();
}

void EasyWifi::wait_for_wifi(uint32_t time_s) {
  const TickType_t ticks_to_wait = Delay::ms_to_ticks(time_s * 1000);
  ESP_LOGI(TAG, "Blocking for wifi connect, %i seconds max", time_s);
  xEventGroupWaitBits(wifi_event_group, ESP_WIFI_CONN_BIT, pdTRUE, pdFALSE,
                      ticks_to_wait);
  ESP_LOGI(TAG, "Wifi blocking finished");
}

esp_err_t EasyWifi::enable_fallback(uint32_t timeout_s) { return ESP_OK; }

esp_err_t EasyWifi::init_software() {
  ESP_LOGI(TAG, "Initializing software");

  /* Create event group that will handle WiFi actions like start and DC */
  EasyWifi::wifi_event_group = xEventGroupCreate();
  esp_event_loop_init(wifi_event_handler, NULL);
  xEventGroupClearBits(wifi_event_group, ESP_WIFI_CONN_BIT);

  /* Always create config this way */
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  // Initialize adapter in client mode, store network in flash
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
  ESP_ERROR_CHECK(esp_wifi_set_auto_connect(true));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi software init finished");
  return ESP_OK;
}

esp_err_t EasyWifi::init_hardware() {
  ESP_LOGI(TAG, "Initializing adapter");
  tcpip_adapter_init();
  nvs_flash_init();
  ESP_LOGI(TAG, "WiFI adapter init finished");
  return ESP_OK;
}

esp_err_t EasyWifi::wifi_event_handler(void *ctx, system_event_t *event) {
  bool autoconnect;
  esp_wifi_get_auto_connect(&autoconnect);

  // Here we handle WiFi events not related to smart config
  switch (event->event_id) {
    case SYSTEM_EVENT_WIFI_READY:
      ESP_LOGI(TAG, "SYSTEM_EVENT_WIFI_READY");
      break;

    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
      break;

    case SYSTEM_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_CONNECTED");
      break;

    /* Log the IP when connecting */
    case SYSTEM_EVENT_STA_GOT_IP: {
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
      xEventGroupSetBits(wifi_event_group, ESP_WIFI_CONN_BIT);
      break;
    }

    /* Auto reconnect if autoconnect is enabled */
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
      xEventGroupClearBits(wifi_event_group, ESP_WIFI_CONN_BIT);
      break;

    default:
      break;
  }

  return ESP_OK;
}

/**
 *
 * Logic test and information getters
 *
 *
 */

bool EasyWifi::is_valid_config(wifi_config_t *config) {
  return is_valid_ssid(config) && is_valid_psk(config);
}
bool EasyWifi::is_valid_ssid(wifi_config_t *config) {
  /* SSID should have max of 32 alphanumerics, including - or _ */
  uint8_t *ssid = config->sta.ssid;
  return ssid != nullptr && ssid[0] != '\0';
}
bool EasyWifi::is_valid_psk(wifi_config_t *config) {
  // TODO this needs to check length
  uint8_t *psk = config->sta.password;
  if (psk == nullptr) {
    return false;
  }
  return true;
}

bool EasyWifi::is_connected() {
  return xEventGroupGetBits(wifi_event_group) & ESP_WIFI_CONN_BIT;
}

ip4_addr_t EasyWifi::getIP() {
  tcpip_adapter_ip_info_t ip = getConnectionInfo();
  ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ip.ip));
  ESP_LOGI(TAG, "MASK:" IPSTR, IP2STR(&ip.netmask));
  ESP_LOGI(TAG, "GW:" IPSTR, IP2STR(&ip.gw));
  return ip.ip;
}

tcpip_adapter_ip_info_t EasyWifi::getConnectionInfo() {
  /* Prepare containing data structure */
  tcpip_adapter_ip_info_t ip;
  memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));

  esp_err_t result = tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Got code %i when getting adapter info", result);
  }
  return ip;
}