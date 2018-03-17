#ifdef UNIT_TEST
#include "unity.h"
#include "SmartConfig/SmartConfig.h"

extern "C" {
void app_main();
}

void init_test() {
	TEST_ASSERT_EQUAL(ESP_OK, SmartConfig.begin(60 * 1000));
	vTaskDelay(10000);
	ip4_addr_t ipv4 = SmartConfig.ip();
	ESP_LOGI("test", "IP:" IPSTR, IP2STR(&ipv4));
}

void init_test2() {
	TEST_ASSERT_EQUAL(ESP_OK, SmartConfig.forceSmartConfig(60 * 1000));
}

void test_task(void *) {
	UNITY_BEGIN();
	RUN_TEST(init_test);
	// RUN_TEST(init_test2);

	UNITY_END();
	vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif