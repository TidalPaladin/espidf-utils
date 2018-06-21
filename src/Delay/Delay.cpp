#include "Delay/Delay.h"

void Delay::delay(uint32_t ms) { vTaskDelay(ms_to_ticks(ms)); }

void Delay::delay_microseconds(uint32_t us) {
  vTaskDelay(ms_to_ticks(us / 1000));
}

void Delay::delay_until(TickType_t ticks_to_wait, TickType_t *prev_wake_time) {
  // If the prev_wake_time was null, assume it to be now
  if (ticks_to_wait) {
    vTaskDelayUntil(prev_wake_time, ticks_to_wait);
  }
}

void Delay::delay_until_ms(uint32_t period_ms, TickType_t *prev_wake_time) {
  delay_until(ms_to_ticks(period_ms), prev_wake_time);
}

TickType_t Delay::ms_to_ticks(uint32_t ms) { return ms / portTICK_PERIOD_MS; }

uint32_t Delay::ticks_to_ms(TickType_t ticks) {
  return ticks * portTICK_PERIOD_MS;
}
