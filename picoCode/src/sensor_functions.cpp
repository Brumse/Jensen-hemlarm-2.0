
#include "pico/stdlib.h"
#include <stdio.h>
#include "sensor_config.h"
#include "sensor_functions.h"


void trigger_pulse()
{   
        gpio_put(TRIG_PIN, 0); 
        sleep_us(5);
        gpio_put(TRIG_PIN, 1);
        sleep_us(10);
        gpio_put(TRIG_PIN,0);
}   

float get_distance()
{
    trigger_pulse();

    // Wait for echo pulse
    while(gpio_get(ECHO_PIN) == 0) tight_loop_contents();
    absolute_time_t start_time = get_absolute_time();

    while(gpio_get(ECHO_PIN) == 1) tight_loop_contents();
    absolute_time_t end_time = get_absolute_time();

    // Calculate duration in microseconds
    int64_t pulse_duration = absolute_time_diff_us(start_time, end_time);
    return (pulse_duration * 0.0343) / 2;
}

