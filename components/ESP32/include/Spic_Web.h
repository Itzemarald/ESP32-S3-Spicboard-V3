#pragma once

#include <stdint.h>
#include "esp_http_server.h"
#include "esp_event.h"

class Spic_Web {
public:
    enum WebCommand { WEB_MODE, WEB_BTN0, WEB_BTN1, WEB_POTI, WEB_PHOTO, NONE };

    typedef void (*WEBCALLBACK)(WebCommand cmd, uint8_t value);

    Spic_Web(void);
    ~Spic_Web(void);
    
    int8_t sb_web_init(const char* SSID, const char* password);

    void sb_web_registerCallback(WEBCALLBACK callback);

    static WEBCALLBACK _callback;

private:
    httpd_handle_t _server = NULL;

    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    static esp_err_t root_get_handler(httpd_req_t* req);
    static esp_err_t api_get_handler(httpd_req_t *req);
};