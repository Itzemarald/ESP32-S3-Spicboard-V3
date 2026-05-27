#include "Spic_Web.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_log.h"
#include <string.h>

Spic_Web::WEBCALLBACK Spic_Web::_callback = NULL;

const char* html_page = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SPICboard Simulator</title>
    <style>
        :root { --pcb-green: #1a4d2e; --display-bg: #111; --display-text: #ff3333; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #2c2c2c; color: white; display: flex; flex-direction: column; align-items: center; margin: 0; padding: 20px; }
        h1 { margin-bottom: 10px; }
        .mode-selector { display: flex; gap: 15px; margin-bottom: 30px; }
        .mode-btn { padding: 10px 20px; font-size: 16px; font-weight: bold; border: 2px solid #555; background-color: #333; color: white; border-radius: 8px; cursor: pointer; transition: all 0.2s; }
        .mode-btn.active { background-color: #007bff; border-color: #007bff; box-shadow: 0 0 10px rgba(0, 123, 255, 0.5); }
        .board { background-color: var(--pcb-green); border: 2px solid #0f2e1b; border-radius: 15px; padding: 30px; display: flex; gap: 50px; box-shadow: 0 15px 30px rgba(0,0,0,0.5), inset 0 0 20px rgba(0,0,0,0.5); position: relative; }
        .board::before, .board::after { content: ''; position: absolute; width: 10px; height: 10px; background: silver; border-radius: 50%; box-shadow: inset 1px 1px 2px #444; }
        .board::before { top: 10px; left: 10px; } .board::after { bottom: 10px; right: 10px; }
        .led-column { display: flex; flex-direction: column-reverse; gap: 12px; align-items: center; background: rgba(0,0,0,0.2); padding: 10px; border-radius: 10px; }
        .led-container { display: flex; align-items: center; gap: 10px; }
        .led-label { font-size: 12px; font-family: monospace; color: #aaa;}
        .led { width: 20px; height: 20px; border-radius: 50%; background-color: currentColor; opacity: 0.15; transition: opacity 0.1s; }
        .led.on { opacity: 1; box-shadow: 0 0 15px currentColor; }
        #led-0, #led-4 { color: #ff3333; } #led-1, #led-5 { color: #ffcc00; } #led-2, #led-6 { color: #33cc33; } #led-3, #led-7 { color: #3399ff; } 
        .display-column { display: flex; flex-direction: column; align-items: center; justify-content: center; }
        .seven-segment { background-color: var(--display-bg); color: var(--display-text); font-family: 'Courier New', Courier, monospace; font-size: 90px; font-weight: bold; padding: 20px 30px; border-radius: 10px; border: 4px solid #000; box-shadow: inset 0 0 15px rgba(255,0,0,0.2); letter-spacing: 5px; line-height: 1; }
        .controls-column { display: flex; flex-direction: column; gap: 25px; justify-content: center; }
        .control-group { background: rgba(0,0,0,0.3); padding: 15px; border-radius: 10px; text-align: center; }
        .control-group label { display: block; margin-bottom: 10px; font-size: 14px; font-weight: bold; color: #ddd; }
        .hw-btn { width: 50px; height: 50px; border-radius: 50%; border: 4px solid #222; background: #444; color: white; font-weight: bold; cursor: pointer; margin: 0 10px; box-shadow: 0 5px 0 #111; transition: all 0.1s; }
        .hw-btn:active:not(:disabled) { transform: translateY(5px); box-shadow: 0 0 0 #111; background: #555; }
        .hw-btn:disabled, input[type=range]:disabled { opacity: 0.3; cursor: not-allowed; }
        input[type=range] { width: 150px; }
    </style>
</head>
<body>
    <h1>SPICboard Simulator</h1>
    <div class="mode-selector">
        <button class="mode-btn active" onclick="setMode(0)" id="btn-mode-0">Mode 0 (Timer)</button>
        <button class="mode-btn" onclick="setMode(1)" id="btn-mode-1">Mode 1 (Poti)</button>
        <button class="mode-btn" onclick="setMode(2)" id="btn-mode-2">Mode 2 (Photo)</button>
    </div>
    <div class="board">
        <div class="led-column" id="led-bar"></div>
        <div class="display-column"><div class="seven-segment" id="display">00</div></div>
        <div class="controls-column">
            <div class="control-group">
                <label>Taster</label>
                <button class="hw-btn" id="hw-btn-1" onclick="handleBtn1()">B1<br><small>(Reset)</small></button>
                <button class="hw-btn" id="hw-btn-0" onclick="handleBtn0()">B0<br><small>(Start)</small></button>
            </div>
            <div class="control-group">
                <label>Potentiometer: <span id="poti-val-lbl">0</span></label>
                <input type="range" id="poti-slider" min="0" max="99" value="0" oninput="handlePoti()">
            </div>
            <div class="control-group">
                <label>Photoresistor: <span id="photo-val-lbl">0</span></label>
                <input type="range" id="photo-slider" min="0" max="99" value="0" oninput="handlePhoto()">
            </div>
        </div>
    </div>
    <script>
        let currentMode = 0;
        let timerValue = 0; let timerRunning = false; let timerInterval = null; let mode0LedMask = 0x00; 
        let potiValue = 0; let photoValue = 0;
        let isInitialized = false;

        const display = document.getElementById('display');
        const ledBar = document.getElementById('led-bar');
        const hwBtn0 = document.getElementById('hw-btn-0'); const hwBtn1 = document.getElementById('hw-btn-1');
        const potiSlider = document.getElementById('poti-slider'); const photoSlider = document.getElementById('photo-slider');
        const potiLbl = document.getElementById('poti-val-lbl'); const photoLbl = document.getElementById('photo-val-lbl');

        for (let i = 7; i >= 0; i--) {
            const container = document.createElement('div'); container.className = 'led-container';
            container.innerHTML = `<div class="led-label">L${i}</div><div class="led" id="led-${i}"></div>`;
            ledBar.appendChild(container);
        }

        function sendEspCommand(cmd, val = 0) {
            if(!isInitialized) return;
            fetch(`/api?cmd=${cmd}&val=${val}`).catch(e => console.error("Network error:", e));
        }

        function updateDisplay(value) { display.innerText = value.toString().padStart(2, '0'); }
        function updateLedsWithMask(mask) {
            for (let i = 0; i < 8; i++) {
                const led = document.getElementById(`led-${i}`);
                if ((mask & (1 << i)) !== 0) {
                    led.classList.add('on');
                } else {
                    led.classList.remove('on');
                }
            }
        }
        function updateLedsBarGraph(value) {
            let numLedsOn = Math.ceil((value / 99) * 8); 
            if (value === 0) numLedsOn = 0;
            for (let i = 0; i < 8; i++) {
                const led = document.getElementById(`led-${i}`);
                if (i < numLedsOn) {
                    led.classList.add('on'); 
                } else {
                    led.classList.remove('on');
                }
            }
        }

        function handleBtn0() {
            if (currentMode !== 0) return;
            sendEspCommand('btn0');
            timerRunning = !timerRunning;
            if (timerRunning) {
                hwBtn0.innerHTML = 'B0<br><small>(Stop)</small>';
                timerInterval = setInterval(() => { 
                                timerValue++; 
                                if (timerValue > 99) timerValue = 0; 
                                    updateDisplay(timerValue); 
                                }, 1000);
            } else {
                hwBtn0.innerHTML = 'B0<br><small>(Start)</small>'; clearInterval(timerInterval);
            }
            mode0LedMask = ((mode0LedMask << 1) | 0x01) & 0xFF; updateLedsWithMask(mode0LedMask);
        }

        function handleBtn1() {
            if (currentMode !== 0) return;
            sendEspCommand('btn1');
            timerValue = 0; updateDisplay(timerValue);
            mode0LedMask = mode0LedMask >> 1; updateLedsWithMask(mode0LedMask);
        }

        function handlePoti() { 
            potiValue = parseInt(potiSlider.value); 
            potiLbl.innerText = potiValue; 
            sendEspCommand('poti', potiValue);
            if (currentMode === 1) { 
                updateDisplay(potiValue); 
                updateLedsBarGraph(potiValue); 
            } 
        }
        function handlePhoto() { 
            photoValue = parseInt(photoSlider.value); 
            photoLbl.innerText = photoValue; 
            sendEspCommand('photo', photoValue);
            if (currentMode === 2) { 
                updateDisplay(photoValue); 
                updateLedsBarGraph(photoValue); 
            } 
        }

        function setMode(mode) {
            currentMode = mode;
            sendEspCommand('mode', mode);

            document.querySelectorAll('.mode-btn').forEach(btn => btn.classList.remove('active'));
            document.getElementById(`btn-mode-${mode}`).classList.add('active');

            if (mode !== 0 && timerRunning) { 
                timerRunning = false; 
                clearInterval(timerInterval); 
                hwBtn0.innerHTML = 'B0<br><small>(Start)</small>'; 
            }
            hwBtn0.disabled = (mode !== 0); 
            hwBtn1.disabled = (mode !== 0); 
            potiSlider.disabled = (mode !== 1); 
            photoSlider.disabled = (mode !== 2);
            if (mode === 0) { 
                updateDisplay(timerValue); 
                updateLedsWithMask(mode0LedMask);
            } 
            else if (mode === 1) { 
                updateDisplay(potiValue); 
                updateLedsBarGraph(potiValue); 
            } 
            else if (mode === 2) { 
                updateDisplay(photoValue); 
                updateLedsBarGraph(photoValue); 
            }
        }
        
        isInitialized = true;
        setMode(0);
    </script>
</body>
</html>
)rawliteral";

Spic_Web::Spic_Web(void) {}
Spic_Web::~Spic_Web(void) {}

void Spic_Web::sb_web_registerCallback(WEBCALLBACK callback) {
    _callback = callback;
}

void Spic_Web::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW("WEB", "Lost WLAN Connection. Try to reconnect...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WEB", "=======================================");
        ESP_LOGI("WEB", " Successfully connected! ");
        ESP_LOGI("WEB", " IP-Adresse: http://" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI("WEB", "=======================================");
    }
}

esp_err_t Spic_Web::root_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t Spic_Web::api_get_handler(httpd_req_t *req) {
    char buf[100];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char cmd_param[20];
        char val_param[10];
        
        if (httpd_query_key_value(buf, "cmd", cmd_param, sizeof(cmd_param)) == ESP_OK) {
            WebCommand cmd_type = NONE;
            uint8_t val = 0;

            if (strcmp(cmd_param, "mode") == 0) {
                cmd_type = WEB_MODE;
                if (httpd_query_key_value(buf, "val", val_param, sizeof(val_param)) == ESP_OK) val = atoi(val_param);
            } else if (strcmp(cmd_param, "btn0") == 0) {
                cmd_type = WEB_BTN0;
            } else if (strcmp(cmd_param, "btn1") == 0) {
                cmd_type = WEB_BTN1;
            } else if (strcmp(cmd_param, "poti") == 0) {
                cmd_type = WEB_POTI;
                if (httpd_query_key_value(buf, "val", val_param, sizeof(val_param)) == ESP_OK) val = atoi(val_param);
            } else if (strcmp(cmd_param, "photo") == 0) {
                cmd_type = WEB_PHOTO;
                if (httpd_query_key_value(buf, "val", val_param, sizeof(val_param)) == ESP_OK) val = atoi(val_param);
            }

            if (_callback) _callback(cmd_type, val);
        }
    }
    
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

int8_t Spic_Web::sb_web_init(const char* SSID, const char* password) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WEB", "Connecting to SSID: %s", SSID);

    httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&_server, &httpd_config) != ESP_OK) {
        return -1;
    }

    httpd_uri_t uri_root = {};
    uri_root.uri = "/";
    uri_root.method = HTTP_GET;
    uri_root.handler = root_get_handler;
    uri_root.user_ctx = NULL;
    httpd_register_uri_handler(_server, &uri_root);

    httpd_uri_t uri_api = {};
    uri_api.uri = "/api";
    uri_api.method = HTTP_GET;
    uri_api.handler = api_get_handler;
    uri_api.user_ctx = NULL;
    httpd_register_uri_handler(_server, &uri_api);

    return 0;
}