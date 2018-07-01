#include "esp_button.h"

/**
 *
 * NOTE:
 * Use of low / high level interrupts is preferred to edge interrupts. This prevents
 * situations where the button may think it is in one state, but actually in another.
 * Coupling to the level, rather than an edge, means that the button will always know if
 * it is pressed or released
 * 
 * ISR bound to low or high level with pull resistor automatically set
 *
 * Interrupts disabled for given button in ISR for debouncing
 * Later processing requested using RTOS daemon / task
 *
 * Processor discriminates press vs release
 * Processor starts timer to re-enable interrupt after deboune period
 *
 *
 */

typedef volatile struct {
    button_config_t xConfig;
    TickType_t xTickCountRelease;
    TickType_t xTickCountPress;
    gpio_int_type_t eLastInterrupt;
} button_handler_t;

TaskHandle_t xButtonTask;
QueueHandle_t xButtonQueue;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/* The handler for GPIO interrupts */
static void vButtonInterruptHandler(void* pvParm)
{
    portENTER_CRITICAL_ISR(&mux);
    vButtonDebugFlip();

    button_handler_t* pxButton = (button_handler_t*)pvParm;
    pxButton->eLastInterrupt = eButtonIntrType(pxButton->xConfig.gpio);
    if (pxButton->eLastInterrupt == GPIO_INTR_LOW_LEVEL) {
        gpio_set_intr_type(pxButton->xConfig.gpio, GPIO_INTR_HIGH_LEVEL);
    } else {
        gpio_set_intr_type(pxButton->xConfig.gpio, GPIO_INTR_LOW_LEVEL);
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(xButtonQueue, (void*)pxButton, &xHigherPriorityTaskWoken);

    portEXIT_CRITICAL_ISR(&mux);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static uint32_t ulGetPressDurationMs(button_handler_t* pxButton)
{
    return portTICK_PERIOD_MS * (pxButton->xTickCountRelease - pxButton->xTickCountPress);
}

static void vButtonEventTask(void* pvParm)
{
    button_handler_t* pxButtonQueued = NULL;
    volatile button_config_t* pxConfig = NULL;

    while (1) {
        /* Wait for button to be queued from ISR trigger*/
        xQueueReceive(xButtonQueue, (void*)&pxButtonQueued, portMAX_DELAY);
        ESP_LOGI(buttonTAG, "Received button event");

        pxConfig = &pxButtonQueued->xConfig;
        const gpio_int_type_t ePressInterrupt = pxConfig->type;
        const gpio_int_type_t eIncomingInterrupt = eButtonIntrType(pxConfig->gpio);

        if (ePressInterrupt == eIncomingInterrupt) {
            /* Button depressed, just set counter */
            pxButtonQueued->xTickCountPress = xTaskGetTickCount();
        } else {
            /* Button released, calculate time button was held */
            pxButtonQueued->xTickCountRelease = xTaskGetTickCount();
            const uint32_t ulPressDurationMs = ulGetPressDurationMs(pxButtonQueued);

            if (ulPressDurationMs < pdMS_TO_TICKS(20)) {
                // NO-OP, pulse too short
            } else if (ulPressDurationMs >= pxConfig->hold_ms) {
                ESP_LOGI(buttonTAG, "HOLD");
                pxConfig->callback(BUTTON_HOLD);
            } else {
                ESP_LOGI(buttonTAG, "PRESS");
                pxConfig->callback(BUTTON_PRESS);
            }
        }
    }
}

esp_err_t xButtonDefaultConfig(button_config_t* pxConfig)
{
    if (pxConfig == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    pxConfig->gpio = GPIO_NUM_MAX;
    pxConfig->hold_ms = buttonDEFAULT_HOLD_MS;
    return ESP_OK;
}

esp_err_t xButtonInit()
{
    ESP_LOGI(buttonTAG, "Registering isr");

#ifdef BUTTON_DEBUG_PIN
    gpio_set_direction(BUTTON_DEBUG_PIN, GPIO_MODE_INPUT_OUTPUT);
#endif

    /* Create the task / queue debounce system */
    xButtonQueue = xQueueCreate(buttonQUEUE_SIZE, sizeof(button_handler_t*));
    xTaskCreate(vButtonEventTask, "but_event", 4096, NULL, 10, &xButtonTask);

    /* Use gpio_install_isr_service() for per GPIO ISR */
    buttonCHECK(gpio_install_isr_service(buttonINTR_FLAGS));
    return ESP_OK;
}

esp_err_t xButtonAdd(button_config_t* pxConfig)
{
    if (pxConfig == NULL) {
        ESP_LOGE(buttonTAG, "Couldn't add button, null pxConfig");
        return ESP_ERR_INVALID_ARG;
    }
    // ESP_LOGI(buttonTAG, "Adding button on GPIO %i", (uint8_t)pxConfig->gpio);

    /* Allocate and store button info */
    button_handler_t* pxHandler = (button_handler_t*)malloc(sizeof(button_handler_t));
    memcpy((void*)&pxHandler->xConfig, (void*)pxConfig,
        sizeof(button_config_t));

    /* Enable interrupt on GPIO */
    buttonCHECK(gpio_set_direction(pxConfig->gpio, GPIO_MODE_INPUT));
    buttonCHECK(gpio_set_pull_mode(pxConfig->gpio, GPIO_PULLUP_PULLDOWN));
    if (pxConfig->type == GPIO_INTR_LOW_LEVEL) {
        buttonCHECK(gpio_pullup_en(pxConfig->gpio));
    } else {
        buttonCHECK(gpio_pulldown_en(pxConfig->gpio));
    }

    /* Add button to ISR handler and create task */
    buttonCHECK(gpio_isr_handler_add(pxConfig->gpio, vButtonInterruptHandler,
        (void*)pxHandler));
    buttonCHECK(gpio_set_intr_type(pxConfig->gpio, pxConfig->type));
    buttonCHECK(gpio_intr_enable(pxConfig->gpio));
    return ESP_OK;
}

esp_err_t xButtonRemove(gpio_num_t gpio)
{
    ESP_LOGI(buttonTAG, "Removing button on GPIO %i", gpio);
    buttonCHECK(gpio_set_intr_type(gpio, GPIO_INTR_DISABLE));
    buttonCHECK(gpio_intr_disable(gpio));
    buttonCHECK(gpio_isr_handler_remove(gpio));
    return ESP_OK;
}

esp_err_t xButtonDeinit()
{
    ESP_LOGI(buttonTAG, "Unregistering isr");
    gpio_uninstall_isr_service();
    vTaskDelete(xButtonTask);
    vQueueDelete(xButtonQueue);
    return ESP_OK;
}
