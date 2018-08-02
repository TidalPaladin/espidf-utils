/**
 * Private helper methods for esp_button.h
 *
 * Following the Google style guide
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

static TimerHandle_t kButtonIntrTimer;

/* Used for portENTER_CRITICAL_ISR */
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* For debug logging */
static const char* const buttonTAG = "button";

static const uint32_t kButtonIntrFlags = ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LOWMED;

/* Button config wrapped into a struct with private members */
typedef struct {
    const button_config_t button_config_;
    TickType_t event_times[2];
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
static inline gpio_int_type_t UpdateInterruptType(button_handler_t* button)
{
    assert(button != NULL && "Null button pointer");
    assert(button->current_interrupt_type_ != GPIO_INTR_DISABLE);

    gpio_int_type_t new_interrupt;
    if (button->current_interrupt_type_ == GPIO_INTR_LOW_LEVEL) {
        new_interrupt = GPIO_INTR_HIGH_LEVEL;
    } else {
        new_interrupt = GPIO_INTR_LOW_LEVEL;
    }
    button->current_interrupt_type_ = new_interrupt;
    return new_interrupt;
}

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
static inline BaseType_t ButtonHandleDepressFromIsr(button_handler_t* handler)
{
    handler->tick_count_press_ = xTaskGetTickCountFromISR();

    /* Enable interrupts upon exit of ISR for button depress */
    gpio_set_intr_type(handler->button_config_.gpio_,
        handler->current_interrupt_type_);
    return pdFALSE;
}

/**
 * @brief Handle ISR events for a button release
 *
 * @param handler   The button handler for the button that was pressed
 *
 * @return pdTRUE if a higher priority task was woken, pdFALSE otherwise
 */
static inline BaseType_t ButtonHandleReleaseFromIsr(button_handler_t* handler)
{
    handler->tick_count_release_ = xTaskGetTickCountFromISR();

    void* enqueue_ptr = (void*)&handler;

    /* Push button to task for deferred processing of release */
    BaseType_t higher_prio_task_woken = pdFALSE;
    xQueueSendFromISR(kButtonQueue, enqueue_ptr, &higher_prio_task_woken);
    return higher_prio_task_woken;
}

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
    volatile button_handler_t* button)
{
    return portTICK_PERIOD_MS * (button->tick_count_release_ - button->tick_count_press_);
}

/**
 * @brief Processes actions to take on button release
 *
 * @param button  Pointer to the button to process
 *
 */
void ButtonProcessRelease(volatile button_handler_t* const button)
{
    assert(button != NULL);
    heap_caps_check_integrity_all(true);

    const uint32_t kElapsedMs = GetPressDurationMillis(button);

    if (kElapsedMs < BUTTON_DEBOUNCE_MS) {
        // NO-OP, pulse too short
        //    ESP_LOGD(buttonTAG, "Ignoring button bounce");
        return;
    }

    /**
   * NOTE: Inserting ESP_LOGX calls into this method causes callback method
   * call to fail. Volatile?
   */

    /* Pulse was of valid length, run press / hold callback */
    assert(button->button_config_.callback_ != NULL);
    if (kElapsedMs >= button->button_config_.hold_ms_) {
        (button->button_config_.callback_)(kButtonHold);
    } else {
        (button->button_config_.callback_)(kButtonPress);
    }

    // ESP_LOGD(buttonTAG, "Finished processing event, duration of %i ms",
    //         kElapsedMs);
}

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
static inline button_handler_t* CopyConfigToHeap(button_config_t* config)
{
    /* Allocate heap for button handler */
    const size_t kSize = sizeof(button_handler_t);
    button_handler_t* handler = (button_handler_t*)malloc(kSize);

    assert(handler != NULL);

    /* Copy supplied config into a handler; Copy handler to heap */
    void* dest = (void*)&handler->button_config_;
    const void* kSrc = (void*)config;
    memcpy(dest, kSrc, sizeof(*config));

    handler->current_interrupt_type_ = config->type_;
    return handler;
}

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
static inline esp_err_t SetPullResistors(const button_config_t* config)
{
    buttonCHECK(gpio_set_pull_mode(config->gpio_, GPIO_PULLUP_PULLDOWN));
    if (config->type_ == GPIO_INTR_LOW_LEVEL) {
        buttonCHECK(gpio_pullup_en(config->gpio_));
    } else {
        buttonCHECK(gpio_pulldown_en(config->gpio_));
    }
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

    buttonCHECK(
        gpio_isr_handler_add(kGpio, ButtonInterruptCallback, (void*)handler));
    buttonCHECK(gpio_set_intr_type(kGpio, kConfig->type_));
    buttonCHECK(gpio_intr_enable(kGpio));
    return ESP_OK;
}

/**
 * @brief Waits for a button event, altering the supplied pointer parameter to point to
 * the button_handler_t that triggered the event. Events are triggered when the ISR enqueues
 * a button_handler_t pointer for deferred processing.
 * 
 * @param triggered_button  This will point to the button handler that triggered the event
 * 
 * @return true if an event occurred, false otherwise
 */
static inline bool ButtonWaitForEvent(button_handler_t* triggered_button)
{
    const BaseType_t kReceivedEvent = xQueueReceive(kButtonQueue, (void*)&triggered_button, portMAX_DELAY);
    if (kReceivedEvent == pdFALSE) {
        ESP_LOGE(buttonTAG, "xQueueReceive expired without getting anything");
    }
    assert(triggered_button != NULL);
    return kReceivedEvent == pdTRUE;
}

static TickType_t GetTimeBetweenEvents(button_handler_t* button)
{
    return button->event_times[1] - button->event_times[0];
}

static void EnqueueEventTime(button_handler_t* button)
{
    button->event_times[0] = button->event_times[1];
    button->event_times[1] = xTaskGetTickCountFromISR();
}
/**
 * @brief Starts a timer that will re-enable
 */

#ifdef __cplusplus
}
#endif

#endif