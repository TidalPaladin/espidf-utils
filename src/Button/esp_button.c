#include "esp_button.h"
#include "button_static.h"

/**
 *
 * Following the Google style guide
 *
 * NOTE:
 * Use of low / high level interrupts is preferred to edge interrupts. This
 * prevents situations where the button may think it is in one state, but
 * actually in another. Coupling to the level, rather than an edge, means that
 * the button will always know if it is pressed or released
 *
 * ISR bound to low or high level with pull resistor automatically set
 *
 * Interrupts disabled for given button in ISR for debouncing
 * Later processing requested using RTOS daemon / task
 *
 * Processor discriminates press vs release
 * Processor starts timer to re-enable interrupt after debounce period
 *
 *
 */

esp_err_t ButtonDefaultConfig(button_config_t* button_config)
{
    assert(button_config != NULL);
    button_config->gpio_ = GPIO_NUM_MAX;
    button_config->hold_ms_ = BUTTON_HOLD_MS;
    return ESP_OK;
}

esp_err_t ButtonInit()
{
    ESP_LOGI(buttonTAG, "Registering isr");
    assert(BUTTON_STACK_SIZE >= configMINIMAL_STACK_SIZE && BUTTON_QUEUE_SIZE > 0);

    kButtonQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(button_handler_t*));
    xTaskCreate(ButtonEventTask, "but_event", 1024, NULL, 10, &kButtonTask);

    assert(kButtonQueue != NULL && kButtonTask != NULL);

    /* Use gpio_install_isr_service() for per GPIO ISR */
    buttonCHECK(gpio_install_isr_service(kButtonIntrFlags));
    return ESP_OK;
}

esp_err_t ButtonAdd(button_config_t* button_config)
{
    assert(button_config != NULL);
    ESP_LOGI(buttonTAG, "Adding button on GPIO %i",
        (uint8_t)button_config->gpio_);

    button_handler_t* handler = CopyConfigToHeap(button_config);
    assert(handler != NULL);

    buttonCHECK(gpio_set_direction(button_config->gpio_, GPIO_MODE_INPUT));
    buttonCHECK(SetPullResistors(button_config));
    buttonCHECK(AddInterruptsForButton(handler));
    return ESP_OK;
}

esp_err_t ButtonRemove(const gpio_num_t gpio_)
{
    ESP_LOGI(buttonTAG, "Removing button on GPIO %i", gpio_);
    buttonCHECK(gpio_set_intr_type(gpio_, GPIO_INTR_DISABLE));
    buttonCHECK(gpio_intr_disable(gpio_));
    buttonCHECK(gpio_isr_handler_remove(gpio_));
    return ESP_OK;
}

esp_err_t ButtonDeinit()
{
    ESP_LOGI(buttonTAG, "Unregistering isr");
    gpio_uninstall_isr_service();
    vTaskDelete(kButtonTask);
    vQueueDelete(kButtonQueue);
    assert(kButtonTask == NULL && kButtonQueue == NULL);
    return ESP_OK;
}

/**
 *
 *
 * Begin static methods
 *
 *
 */

static void ButtonInterruptCallback(void* pvParm)
{
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

static void ButtonEventTask(void* unused /* Not used */)
{
    static button_handler_t* button;

    /* Begin blocking loop */
    while (1) {

        const bool kReceivedEvent = ButtonWaitForEvent(button);
        heap_caps_check_integrity_all(true);

        if (!kReceivedEvent) {
            continue;
        }

        assert(button != NULL);
        ButtonProcessRelease(button);

        /* Re-enable enterrupt based on value set from UpdateInterruptType() */
        gpio_set_intr_type(button->button_config_.gpio_,
            button->button_config_.type_);
    }
}
