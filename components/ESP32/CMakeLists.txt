idf_component_register(
    SRCS "Spic_Web.cpp" "Spic_UART.cpp"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_gpio esp_driver_uart esp_timer esp_wifi esp_http_server nvs_flash 
)