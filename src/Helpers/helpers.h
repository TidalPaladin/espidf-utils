/**
 * Collection of helper methods designed to simply very basic tasks 
 * on the ESP32 with ESP-IDF.
 * 
 * @author Scott Chase Waggener
 * @date 7/3/18
 */

#ifndef __UTILS_HELPERS_H__
#define __UTILS_HELPERS_H__

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Helper to fetch a stack size expressed as an offset in words from the
 * minimum system stack size for a FreeRTOS task. See configMINIMAL_STACK_SIZE
 * in the FreeRTOS documentation for details
 * 
 * @param ulOffsetWords How many words to add to the minimal stack size
 * 
 * @return configMINIMUM_STACK_SIZE + ulOffsetWords
 */
static uint32_t ulMinStackSizeOffset(uint32_t ulOffsetWords);

#endif