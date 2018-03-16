#ifndef UNIT_TEST
#include "unity.h"
#include "SmartConfig/SmartConfig.h"

void init_test() {
	TEST_ASSERT_EQUAL(ESP_OK, SmartConfig.begin());
	vTaskDelay(100);
}

void test_task(void *) {

	RUN_TEST(init_test);

	while (true) {
	}
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif