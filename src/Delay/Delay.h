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
void delay(uint32_t ms);

/**
 * @brief Delays the task for a given number of microseconds
 *
 * @param ms    How many microseconds to delay the task for
 *
 */
void delay_microseconds(uint32_t us);

/**
 * @brief Delays the task until the given number of ticks have passed between
 *        the given previous wake time and now
 *
 * @attention             Get the current tick counter with xTaskGetTickCount()
 *
 * @param ticks_to_wait   How many ticks should elapse before the delay ends
 * @param prev_wake_time  The base tick counter to which 'ticks_to_wait' is
 *                        relative to
 */
void delay_until(TickType_t ticks_to_wait, TickType_t *prev_wake_time);

/**
 * @brief Delays the task until the given number of milliseconds have passed
 *        between the given previous wake time and now
 *
 * @attention             Get the current tick counter with xTaskGetTickCount()
 *
 * @param ticks_to_wait   How many ticks should elapse before the delay ends
 * @param prev_wake_time  The base tick counter to which 'ticks_to_wait' is
 *                        relative to
 */
void delay_until_ms(uint32_t period_ms, TickType_t *prev_wake_time);

/**
 * @brief Convert a time in milliseconds to the number of ticks that would
 * elapse during that time
 *
 * @param ms  The time in milliseconds to convert
 *
 * @return TickType_t representing how many ticks are in 'ms' milliseconds
 */
TickType_t ms_to_ticks(uint32_t ms);

/**
 * @brief Convert a number of ticks to the time in milliseconds for 'ticks' to
 * happen
 *
 * @param ticks The number of ticks to convert to milliseconds
 *
 * @return The time in milliseconds it takes for 'ticks' ticks to happen
 */
uint32_t ticks_to_ms(TickType_t ticks);

};  // namespace Delay

#endif