#ifndef BUTTON_STATIC_H_
#define BUTTON_STATIC_H_

#include "esp_button.h"

/* Handles deferred processing of buttons after ISR is triggered */
static TaskHandle_t kButtonTask;

/* ISR enqueues to this queue for deferred processing */
static QueueHandle_t kButtonQueue;

/* Used for portENTER_CRITICAL_ISR */
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* For debug logging */
static const char* const buttonTAG = "button";

static const uint32_t kButtonIntrFlags =
    ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LOWMED;

/* Button config wrapped into a struct with private members */
typedef volatile struct {
  button_config_t button_config_;
  TickType_t tick_count_release_;
  TickType_t tick_count_press_;
  gpio_int_type_t current_interrupt_type_;
} button_handler_t;

/**
 * @brief Modifies the interrupt of a GPIO such that the pin
 * will interrupt on the level opposite of it's current setting
 *
 * @param button	Pointer to the button handler
 *
 * post: Interrupt of button->button_config.gpio modifed such that
 * 			GPIO_INTR_LOW_LEVEL -> GPIO_INTR_HIGH_LEVEL
 * 			GPIO_INTR_HIGH_LEVEL -> GPIO_INTR_LOW_LEVEL
 *
 * @return The gpio_int_type_t that is now active for the given button
 */
static gpio_int_type_t SetOppositeInterruptType(button_handler_t* button);

/**
 * @brief Interrupt handler shared across all buttons
 *
 * @param pvParm	Pointer to the button_handler_t that triggered the
 * interrupt
 *
 * post: 	eInvertInterruptType() called on interrupting pin
 * 			Interrupting button queued for deferred processing
 *
 */
static void ButtonInterruptCallback(void* pvParm);

/**
 * @brief Reads the currently assigned press and release tick counters for a
 * button and computes the elapsed press duration in milliseconds.
 *
 * pre: tick_count_press_ and tick_count_release_ assigned to most current value
 * 		tick_count_release_ >= tick_count_press_
 *
 * @param button	Pointer to the button to be processed
 *
 * @return The milliseconds elapsed between tick_count_press_ and
 * tick_count_release_
 */
static inline uint32_t GetPressDurationMillis(button_handler_t* button);

/**
 * @brief Processes actions to take on button release
 *
 * @param button  Pointer to the button to process
 *
 */
static void ButtonProcessRelease(button_handler_t* button);

/**
 * @brief Task used to handle deferred processing of interrupts. Unblocks on
 * enqueue from ISR.
 *
 * @param parm	NULL
 *
 * @return Function never returns
 */
static void ButtonEventTask(void* /* Not used */);

#endif