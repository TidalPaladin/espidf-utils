#ifdef UNIT_TEST
#include "unity.h"
#include "Delay/Delay.h"

extern "C" {
void app_main();
}

void ms_to_ticks() {
	uint32_t ms = 100;
	TickType_t expected_ticks = ms / portTICK_PERIOD_MS;
	TickType_t actual_ticks = Delay.millisecondsToTicks(ms);
	TEST_ASSERT_EQUAL(expected_ticks, actual_ticks);
}

void ticks_to_ms() {
	TickType_t ticks = 100;
	uint32_t expected_ms = ticks * portTICK_PERIOD_MS;
	uint32_t actual_ms = Delay.ticksToMilliseconds(ticks);
	TEST_ASSERT_EQUAL(expected_ms, actual_ms);
}

void delay() {
	uint32_t delay_ms = 100;
	TickType_t start_count = xTaskGetTickCount();
	Delay.delay(delay_ms);
	TickType_t end_count = xTaskGetTickCount();

	uint32_t actual_ms = Delay.ticksToMilliseconds(end_count - start_count);
	TEST_ASSERT_EQUAL(delay_ms, actual_ms);
}

void delay_until_ms() {
	uint32_t delay_ms = 100;
	TickType_t start_count = xTaskGetTickCount();
	Delay.waitForFixedPeriod(delay_ms, &start_count);
	TickType_t end_count = xTaskGetTickCount();

	uint32_t actual_ms = Delay.ticksToMilliseconds(end_count - start_count);
	TEST_ASSERT_EQUAL(delay_ms, actual_ms);
}

void test_task(void *) {

	vTaskDelay(2000 / portTICK_PERIOD_MS);
	RUN_TEST(ms_to_ticks);
	RUN_TEST(ticks_to_ms);
	RUN_TEST(delay);
	// RUN_TEST(delay_until_ms);

	vTaskDelete(NULL);
}

void app_main() { xTaskCreate(test_task, "test", 4096, NULL, 1, NULL); }

#endif