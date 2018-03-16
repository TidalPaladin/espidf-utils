#ifdef UNIT_TEST
#include "unity.h"
#include "SmartConfig/SmartConfig.h"

extern "C" {
void app_main();
}

void init_test() {
	TEST_ASSERT_EQUAL(ESP_OK, SmartConfig.begin(60 * 1000));
	vTaskDelay(100);
}

void test_task(void *) {

	RUN_TEST(init_test);
	vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif