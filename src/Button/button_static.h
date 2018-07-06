/**
 * Private helper methods for esp_button.h
 *
 * @author Scott Chase Waggener
 * @date 7/5/18
 *
 */

#ifndef BUTTON_STATIC_H_
#define BUTTON_STATIC_H_

#include "esp_button.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  const button_config_t button_config_;
  TickType_t tick_count_release_;
  TickType_t tick_count_press_;
  gpio_int_type_t current_interrupt_type_;
} button_handler_t;

/**
 * @brief Modifies a button handler such that the current interrupt
 * tracking variable is inverted.
 *
 * @param button	Pointer to the button handler
 *
 * post: Interrupt tracking variable modifed such that
 * 			GPIO_INTR_LOW_LEVEL -> GPIO_INTR_HIGH_LEVEL
 * 			GPIO_INTR_HIGH_LEVEL -> GPIO_INTR_LOW_LEVEL
 *
 * @return The gpio_int_type_t that is now active for the given button
 */
static gpio_int_type_t UpdateInterruptType(volatile button_handler_t* button);

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
 * @brief Handle ISR events for a button depress
 *
 * @param handler   The button handler for the button that was pressed
 *
 * @return BaseType_t pdTRUE
 */
static inline BaseType_t ButtonIsrHandleDepress(
    volatile button_handler_t* handler);

/**
 * @brief Handle ISR events for a button release
 *
 * @param handler   The button handler for the button that was pressed
 *
 * @return pdTRUE if a higher priority task was woken, pdFALSE otherwise
 */
static inline BaseType_t ButtonIsrHandleRelease(
    volatile button_handler_t* handler);

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
static inline uint32_t GetPressDurationMillis(
    volatile button_handler_t* button);

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

/**
 * @brief Copies a button config supplied by the user into heap
 *
 * @param config    The config for the button to be copied
 *
 * @return Pointer to the button handler in heap
 */
static inline button_handler_t* CopyConfigToHeap(const button_config_t* config);

/**
 * @brief Sets the and pull resistors during button creation
 *
 * @param config    The config for the button being added
 *
 * post:    Button's pull resistors set to pull to level opposite that of the
 *          interrupt type that indicates button depress.
 *          Ex. type_ = GPIO_INTR_LOW_LEVEL sets pull resistors to high
 *
 * @return esp_err_t
 */
static inline esp_err_t SetPullResistors(const button_config_t* config);

/**
 * @brief Adds a new button to the interrupt handler. Called for each newly
 * added button
 *
 * @param config    The config for the button being added
 *
 * @return esp_err_t
 */
static inline esp_err_t AddInterruptsForButton(
    volatile button_handler_t* config);

#ifdef __cplusplus
}
#endif

#endif