#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wifi_credentials.h"

const char* ssid = "SSID"; // Replace with your Wi-Fi SSID
const char* password = "PASSWORD"; // Replace with your Wi-Fi password

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    printf("initialised\n");

    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect\n");
        cyw43_arch_deinit();
	return 1;
    } else {
        printf("Connected to WiFi\n");
    }

    while (true) {
        sleep_ms(1000);
    }

    return 0;
}
