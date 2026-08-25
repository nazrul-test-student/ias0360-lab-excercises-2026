#include <stdio.h>
#include <vector>
#include <cstring> 
#include <sstream>

#include "pico/stdlib.h"
#include "pico/platform.h"
#include "hardware/sync.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/timer.h"

#include "networking_main.h"
#include "request.hpp"

static inline void debug_unpause_timer(void) {
    timer_hw->dbgpause = 0; 
}

// Connect WiFi
int wifi_connection_example(void) {

	stdio_init_all();
    debug_unpause_timer();  

    sleep_ms(5000);

    if (cyw43_arch_init()) {
        printf("Wi-Fi chip init failed\n");
        return -1;
    }
    printf("Wi-Fi chip initialized successfully\n");

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    printf("Start connecting wifi.\n");
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("WiFi connected successfully! \n");

    sleep_ms(3000);

    while (1) tight_loop_contents();
    return 0;
}

// Send GET request
int get_request_example(void) {
	stdio_init_all();
    debug_unpause_timer();  

    sleep_ms(5000);

    if (cyw43_arch_init()) {
        printf("Wi-Fi chip init failed\n");
        return -1;
    }
    printf("Wi-Fi chip initialized successfully\n");

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    printf("Start connecting wifi.\n");
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("WiFi connected successfully! \n");

    sleep_ms(3000);

    const char* get_url = "/api/v1/healthcheck";
    HttpRequest request(get_url);
    ResponseDataStr response = request.get();
    if (response.status_ok) {
        printf("GET Response: %s \n", response.y);
        // Expected output: "healthy"
    } else {
        printf("GET Request failed\n");
    }

    while (1) tight_loop_contents();
    return 0;
}

// Send POST request
int post_request_example(void) {
    stdio_init_all();
    debug_unpause_timer();  

    sleep_ms(5000);

    if (cyw43_arch_init()) {
        printf("Wi-Fi chip init failed\n");
        return -1;
    }
    printf("Wi-Fi chip initialized successfully\n");

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    printf("Start connecting wifi.\n");
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("WiFi connected successfully! \n");

    sleep_ms(3000);

    const char* post_url = "/api/v1/greet";
    HttpRequest request(post_url);

    std::string json;
    json.append("{\"name\": \"");
    std::string name = "Raspberry Pi Pico W";
    json.append(name);
    json.append("\"}");

    request.set_body_raw(json.c_str(), json.size());

    ResponseDataStr response = request.post();
    if (response.status_ok) {
        printf("POST Response: %s \n", response.y);
        // Expected output: "Hello, Raspberry Pi Pico W!"
    } else {
        printf("POST Request failed\n");
    }


    while (1) tight_loop_contents();
    return 0;
}

// TODO: Implement a function that sends POST request:
// url: /api/v1/predict_a
// body: {"x": "<data>"}, where data is float 32 value.
// content-type: application/json
int post_request_with_float_data(void) {
    stdio_init_all();
    debug_unpause_timer();  

    sleep_ms(5000);

    if (cyw43_arch_init()) {
        printf("Wi-Fi chip init failed\n");
        return -1;
    }
    printf("Wi-Fi chip initialized successfully\n");

    cyw43_arch_enable_sta_mode();

    // Connect to the WiFI network - loop until connected
    printf("Start connecting wifi.\n");
    while(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Attempting to connect...\n");
    }
    // Print a success message once connected
    printf("WiFi connected successfully! \n");

    sleep_ms(3000);

    // Set URL
    const char* post_url = "/api/v1/predict_a";
    HttpRequest request(post_url);

    // Construct request body
    std::string json;
    float x = 3.14f;
    json.append("{\"x\": ");
    json.append(std::to_string(x));
    json.append("}");

    // Set body
    request.set_body_raw(json.c_str(), json.size());

    // Send POST request
    ResponseDataStr response = request.post();
    if (response.status_ok) {
        printf("POST Response: %s \n", response.y);
        // Expected output: 2*x + 1
    } else {
        printf("POST Request failed\n");
    }

    while (1) tight_loop_contents();
    return 0;
}

int main(void)
{
    // Check WiFi connection
    return wifi_connection_example(); // Connect WiFi

    // Check GET request example
    // return get_request_example(); // Send GET request

    // Check POST request example
    // return post_request_example(); // Send POST request

    // Check POST request with arbitrary float data
    // return post_request_with_float_data();   // Send POST request with float data
}