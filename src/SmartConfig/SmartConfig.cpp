#include "SmartConfig.h"

const char *SmartConfigStatic::TAG = "SC";
EventGroupHandle_t SmartConfigStatic::wifi_event_group;

esp_err_t SmartConfigStatic::begin() {
	initAdapter();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	// Initialize WiFi and start it in client mode
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t SmartConfigStatic::begin(uint32_t timeout_ms) {
	begin();

	TickType_t xTicksToWait = Delay.millisecondsToTicks(timeout_ms);
	EventBits_t uxBits = xEventGroupWaitBits(
		wifi_event_group,  /* The event group being tested. */
		ESPTOUCH_DONE_BIT, /* The bits within the event group to wait for. */
		pdTRUE,		   /* BIT_0 & BIT_4 should be cleared before returning. */
		pdFALSE,	   /* Don't wait for both bits, either bit will do. */
		xTicksToWait); /* Wait a maximum of 100ms for either bit to be set. */

	if ((uxBits & ESPTOUCH_DONE_BIT) != 0) {
		ESP_LOGI(TAG, "Connection established, leaving begin()");
		return ESP_OK;
	} else {
		ESP_LOGI(TAG, "Connection timeout, leaving begin()");
		return ESP_ERR_TIMEOUT;
	}
}

esp_err_t SmartConfigStatic::initAdapter() {
	ESP_LOGI(TAG, "Initializing adapter");
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, NULL));
	return ESP_OK;
}

esp_err_t SmartConfigStatic::eventHandler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	// On wifi start, launch smart config task
	case SYSTEM_EVENT_STA_START:
		xTaskCreate(smartConfigTask, "smart_config", 4096, NULL, 3, NULL);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, ESPTOUCH_CONNECTED_BIT);
		break;
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
		ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
		ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
		ESP_ERROR_CHECK(esp_wifi_disconnect());
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
		ESP_ERROR_CHECK(esp_wifi_connect());
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
	EventBits_t uxBits;
	ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
	ESP_ERROR_CHECK(esp_smartconfig_start(smartConfigCallback));
	while (1) {
		uxBits = xEventGroupWaitBits(wifi_event_group,
									 ESPTOUCH_CONNECTED_BIT | ESPTOUCH_DONE_BIT,
									 true, false, portMAX_DELAY);
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