#ifdef UNIT_TEST
#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "map"
#include <string>
#include "NVS/NVS.h"

extern "C" {
void app_main();
}

std::string sample = "hello";

template <typename T>
esp_err_t write_readback(const char *key, T input, T readback) {
	esp_err_t result;
	result = NVS.begin();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS begin");

	result = NVS.write(key, input);
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "Write val");

	result = NVS.read("test", write_val_readback);
	if (result != ESP_OK) {
		return result;
	}

	result = NVS.end();
	TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, result, "NVS end");
	return result;
}

template <typename T> void write_readback_test(const char *key, T input) {
	T output;
	esp_err_t result = write_readback(key, input, output);
	if (result == ESP_OK) {
		TEST_ASSERT_EQUAL(input, output);
	}
}

void int8_test() { write_readback_test<int8_t>(__func__, 100); }
void int16_test() { write_readback_test<int16_t>(__func__, 100); }
void int32_test() { write_readback_test<int32_t>(__func__, 100); }
void uint8_test() { write_readback_test<uint8_t>(__func__, 100); }
void uint16_test() { write_readback_test<uint16_t>(__func__, 100); }
void uint32_test() { write_readback_test<uint32_t>(__func__, 100); }
void blob_test1() { write_readback_test<std::string>(__func__, sample); }
void blob_test2() { write_readback_test<double>(__func__, 100.0); }

void test_task(void *) {

	vTaskDelay(2000 / portTICK_PERIOD_MS);
	UNITY_BEGIN();

	RUN_TEST(int8_test);
	RUN_TEST(int16_test);
	RUN_TEST(int32_test);
	RUN_TEST(uint8_test);
	RUN_TEST(uint16_test);
	RUN_TEST(uint32_test);
	RUN_TEST(blob_test1);
	RUN_TEST(blob_test2);

	UNITY_END();
	vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif