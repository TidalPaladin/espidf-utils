/**
 * Implements pulse width measurement functions using a simple interrupt based approach.
 * Alternatives include a MCPWM or remote control based hardware approach
 *
 */

#ifndef __PULSE_WIDTH_INTERRUPT_H__
#define __PULSE_WIDTH_INTERRUPT_H__

#include "PulseWidth.h"


/* Handles deferred processing of buttons after ISR is triggered */
static TaskHandle_t kPulseTask;

static TimerHandle_t kPulseIntrTimer;

/* Used for portENTER_CRITICAL_ISR */
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* For debug logging */
static const char* const kPulseTag = "pwidth";

esp_err_t PulseWidthInit()
{
    ESP_LOGI(pwTAG, "Registering isr");
    xTaskCreate(ButtonEventTask, "pulse_width", 1024, NULL, 10, &kPulseTask);


    /* Use gpio_install_isr_service() for per GPIO ISR */
    pulseCHECK(gpio_install_isr_service(kButtonIntrFlags));
    return ESP_OK;
}



/**
 * @brief Adds a new button to the interrupt handler. Called for each newly
 * added button
 *
 * @param config    The config for the button being added
 *
 * @return esp_err_t
 */
static inline esp_err_t AddInterrupts(button_handler_t* handler)
{
    const button_config_t const* kConfig = &handler->button_config_;
    const gpio_num_t kGpio = kConfig->gpio_;

    pulseCHECK(
        gpio_isr_handler_add(kGpio, ButtonInterruptCallback, (void*)handler));
    pulseCHECK(gpio_set_intr_type(kGpio, kConfig->type_));
    pulseCHECK(gpio_intr_enable(kGpio));
    return ESP_OK;
}

static void PulseWidthIsr(void* param) {
    portENTER_CRITICAL_ISR(&mux);

    /* Disable interrupts ASAP or interrupts will continue to queue */
    button_handler_t* button = (button_handler_t*)pvParm;
    const button_config_t* config = &button->button_config_;
    gpio_set_intr_type(config->gpio_, GPIO_INTR_DISABLE);

    /* Released if ISR trigger doesn't match user defined press interrupt type */
    const bool kWasReleased = (button->current_interrupt_type_ != config->type_);
    UpdateInterruptType(button);

    /* TODO rename variables, this is confusing */
    BaseType_t needs_task_switch;
    if (kWasReleased) {
        needs_task_switch = ButtonHandleReleaseFromIsr(button);
    } else {
        needs_task_switch = ButtonHandleDepressFromIsr(button);
    }

    /* If we woke a higher priority task from ISR, exit ISR to that task */
    portEXIT_CRITICAL_ISR(&mux);
    if (needs_task_switch == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

#endif