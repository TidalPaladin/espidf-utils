#ifdef UNIT_TEST
#include "unity.h"
#include "NVS/NVS.h"

extern "C" {
void app_main();
}

void read_test() {
	esp_err_t result;
	result = NVS.begin();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS begin");

	int32_t read_val = 0;
	result = NVS.read_i32("test", read_val);
	if (result != ESP_OK) return;

	TEST_ASSERT_EQUAL(read_val, 100);
	result = NVS.end();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS end");
}

void write_test() {
	esp_err_t result;
	result = NVS.begin();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS begin");

	int32_t write_val = 100;
	result = NVS.write_i32("test", write_val);
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "Get restart counter");

	int32_t write_val_readback;
	NVS.read_i32("test", write_val_readback);
	TEST_ASSERT_EQUAL(write_val, write_val_readback);

	result = NVS.end();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS end");
}

void test_task(void *) {

	vTaskDelay(2000 / portTICK_PERIOD_MS);
	UNITY_BEGIN();

	RUN_TEST(read_test);
	RUN_TEST(write_test);

	UNITY_END();
	vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif