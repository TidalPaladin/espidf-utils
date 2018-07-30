#ifndef __PULSE_WIDTH_STATIC_H__
#define __PULSE_WIDTH_STATIC_H__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "gpio.h"
#include "soc/gpio_reg.h"
#include "soc/soc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcnt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

static void PulseWidthInitPulseCounter()
{
}

static void PulseWidthInitRemote()
{
}

static void PulseWidthInitInterrupt()
{
}

#endif