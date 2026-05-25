#include "adc_spic.h"

int8_t Spic_ADC::sb_adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config_poti = {};
    init_config_poti.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config_poti, &_adc_handle));

    adc_oneshot_chan_cfg_t adc_config = {};
    adc_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_config.atten = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc_handle, _adc_channel[0], &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc_handle, _adc_channel[1], &adc_config));

    return ESP_OK;
}

int16_t Spic_ADC::sb_adc_read(ADCDEV dev) {
    if (dev != POTI && dev != PHOTO) return -1;
    int adc_value = 0;
    if (adc_oneshot_read(_adc_handle, _adc_channel[dev], &adc_value) != ESP_OK) return -1;

    return (int16_t)adc_value;
}