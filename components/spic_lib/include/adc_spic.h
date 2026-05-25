#pragma once

#include "esp_adc/adc_oneshot.h"
#include <stdint.h>

class Spic_ADC {
public:
enum ADCDEV {
    POTI,
    PHOTO
};
    Spic_ADC(void) = default;
    ~Spic_ADC(void) = default;
    
    int8_t sb_adc_init(void);

    int16_t sb_adc_read(ADCDEV dev);
private:
    adc_channel_t _adc_channel[2] = {ADC_CHANNEL_4, ADC_CHANNEL_3};
    adc_oneshot_unit_handle_t _adc_handle;
};