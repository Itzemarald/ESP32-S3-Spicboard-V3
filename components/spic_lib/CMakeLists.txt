idf_component_register(
    SRCS "timer_spic.cpp" "button_spic.cpp" "led_spic.cpp" "adc_spic.cpp" "7seg_spic.cpp" "timer_spic.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_gpio esp_timer esp_adc
)