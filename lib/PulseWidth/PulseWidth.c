#include "PulseWidth.h"

esp_err_t PulseWidthInit()
{
    ESP_LOGI(pwTAG, "Registering isr");

    xTaskCreate(ButtonEventTask, "pulse_width", 1024, NULL, 10, &kButtonTask);

    assert(kButtonQueue != NULL && kButtonTask != NULL);

    /* Use gpio_install_isr_service() for per GPIO ISR */
    buttonCHECK(gpio_install_isr_service(kButtonIntrFlags));
    return ESP_OK;
}

esp_err_t PulseWidthAdd(pulse_width_config_t* config)
{
    //
    //
}

esp_err_t PulseWidthRemove(gpio_num_t gpio)
{
    //
    //
}

static void PulseWidthIsr(void* arg){
    
}