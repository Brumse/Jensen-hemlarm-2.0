/**
 * @file mqttmain.cpp
 * @brief Main file for Pico W MQTT application
 *
 * Freely adapted from
 * [mqtt.c](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/mqtt/mqtt_client.c)
 */
#include "hardware/i2c.h"
#include "lcd_printer.h"
#include "lwip/apps/mqtt.h"
#include "mqtt_config.h"
#include "mqtt_client_data.h"
#include "mqtt_functions.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "sensor_config.h"
#include "wifi_credentials.h"
#include <cstdio>
#include <lwip/ip4_addr.h>
#include <pico/time.h>
#include "melody.h"
#include "hardware/pwm.h"

/**
 * @brief Good old main
 */
int main(void) {
    // unplugging stabalizer
    sleep_ms(1000);

    stdio_init_all();
    printf("Initializing Pico W...");
    // sensor and lcd init
    // init button pin
    //  initilize GPIO-pins
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(CLEAR_BUTTON_PIN);
    gpio_set_dir(CLEAR_BUTTON_PIN,GPIO_IN);
    gpio_pull_up(CLEAR_BUTTON_PIN);
    
    gpio_init(BUZZER_TRIG_PIN);
    gpio_set_dir(BUZZER_TRIG_PIN, GPIO_OUT);
    gpio_put(BUZZER_TRIG_PIN, 0);

    // Initiera I2C och LCD
#ifdef i2c_default
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN,
                               PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    lcd_init();
    lcd_clear(); // clear screen at start
    lcd_string("Conn to WiFi..");
#endif

    if (cyw43_arch_init()) {
        panic("Failed to initialize CYW43");
    }

    printf("Attempting wifi connect...");
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID.c_str(),
                                           WIFI_PASSWORD.c_str(),
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        panic("Failed to Connect");
    }
#ifdef i2c_default
    lcd_clear();
    lcd_string("WiFi connected!");
    lcd_set_cursor(1, 0);
    lcd_string("Conn to MQTT...");
    sleep_ms(1000);
#endif
    // Setup mqtt client info

    static mqtt_client_data_t state;
    state.alarm_active = false;
    if (!ipaddr_aton(_MQTT_BROKER_IP, &state.mqtt_server_address)) {
        panic("Could not convert broker IP");
    }
    state.mqtt_server_port = _MQTT_PORT;

    const char *will_topic = "/alarm/status";
    char will_msg_buffer[128];
    snprintf(
        will_msg_buffer, sizeof(will_msg_buffer),
        "{\"device_id\": \"%s\", \"name\": \"%s\", \"status\": \"offline\"}",
        MQTT_DEVICE_NAME, MQTT_DEVICE_NAME);
    const char *will_msg = will_msg_buffer;

    state.mqtt_client_info.client_id = "pico_w";
    state.mqtt_client_info.keep_alive = 10;
    state.mqtt_client_info.client_user = NULL;
    state.mqtt_client_info.client_pass = NULL;
    state.mqtt_client_info.will_topic = will_topic;
    state.mqtt_client_info.will_msg = will_msg;
    state.mqtt_client_info.will_qos = 1;
    state.mqtt_client_info.will_retain = 0;

    // Setup the mqtt client and start the mqtt cycle
    start_client(&state);
#ifdef i2c_default
    lcd_clear();
    lcd_string("MQTT Connected!");
    sleep_ms(1000);
    lcd_string("                     ");
#endif

    // Main loop to wait for workers to do job
    while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client)) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
    }
    printf("Done, exiting...");
    return 0;
}
