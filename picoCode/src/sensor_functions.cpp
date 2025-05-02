
#include "sensor_functions.h"
#include "pico/stdlib.h"
#include "sensor_config.h"
#include <stdio.h>

void trigger_pulse() {
    gpio_put(TRIG_PIN, 0);
    sleep_us(5);
    gpio_put(TRIG_PIN, 1);
    sleep_us(10);
    gpio_put(TRIG_PIN, 0);
}

float get_distance() {
    trigger_pulse();
    uint32_t timeout_us = 100000; // timeout (100 ms)
    uint32_t start_time = 0;
    uint32_t end_time = 0;

    uint32_t timer = 0;
    while (gpio_get(ECHO_PIN) == 0 && timer < timeout_us) {
        sleep_us(1);
        timer++;
    }
    if (timer >= timeout_us) {
        printf("Timeout waiting for ECHO HIGH\n");
        return -1.0f; // error
    }
    start_time = time_us_32();

    timer = 0;
    while (gpio_get(ECHO_PIN) == 1 && timer < timeout_us) {
        sleep_us(1);
        timer++;
    }
    if (timer >= timeout_us) {
        printf("Timeout waiting for ECHO LOW\n");
        return -1.0f; // error
    }
    end_time = time_us_32();

    int64_t pulse_duration = end_time - start_time;
    return (pulse_duration * 0.0343f) / 2.0f;
}
