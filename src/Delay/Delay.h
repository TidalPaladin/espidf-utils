/**
 * Library designed to simplify delays on ESP-IDF
 *
 * @author Scott Chase Waggener <tidalpaladin@gmail.com>
 * @date 6/18/18
 */

#ifndef __DELAY_HELPER_H__
#define __DELAY_HELPER_H__

#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

namespace Delay {
/**
 * @brief Delays the task for a given number of milliseconds
 *
 * @param ms    How many milliseconds to delay the task for
 *
 */
void delay(uint32_t ms) { vTaskDelay(millisecondsToTicks(ms)); }

/**
 * @brief Delays the task for a given number of microseconds
 *
 * @param ms    How many microseconds to delay the task for
 *
 */
void delayMicroseconds(uint32_t ms) { vTaskDelay(millisecondsToTicks(ms)); }

void delayUntil(TickType_t ticks_to_wait,
                TickType_t *prev_wake_time = nullptr) {
  // If the prev_wake_time was null, assume it to be now
  if (prev_wake_time == nullptr) {
    prev_wake_time = new TickType_t;
    *prev_wake_time = xTaskGetTickCount();
  }
  if (ticks_to_wait) vTaskDelayUntil(prev_wake_time, ticks_to_wait);
}

void waitForFixedPeriod(uint32_t period_ms,
                        TickType_t *prev_wake_time = nullptr) {
  delayUntil(millisecondsToTicks(period_ms), prev_wake_time);
}

/**
 * @brief Converts a given number of task ticks to milliseconds
 *
 * @param ms    The number of milliseconds to convert to ticks
 *
 * @return
 */
static constexpr TickType_t millisecondsToTicks(uint32_t ms) {
  return ms / portTICK_PERIOD_MS;
}

static constexpr uint32_t ticksToMilliseconds(TickType_t ticks) {
  return ticks * portTICK_PERIOD_MS;
}

};  // namespace Delay

#endif