#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wifi_credentials.h"
#include "string.h"
#include "hardware/i2c.h"
#include "lcd_printer.h"
#include "pico/binary_info.h"
/*
NOTES for adding functions:

	add LCD-display to show text when connected to wifi and mqtt and also MAYBE indicate trigger alarm.
	add arlarmsound when trigger alarm.
	add button to reset alarm.
*/

const uint TRIG_PIN = 2;
const uint ECHO_PIN = 3;
const uint LED_PIN = 15;

// set threshold for alarm
const float ALARM_THRESHOLD = 10.0f;


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


int main()
{
	stdio_init_all();

	// initilize GPIO-pins
	gpio_init(TRIG_PIN);
	gpio_set_dir(TRIG_PIN, GPIO_OUT);
	gpio_init(ECHO_PIN);
	gpio_set_dir(ECHO_PIN, GPIO_IN);
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);
#if !defined(i2c_default) || !defined(PICO_DEFAULT_I2C_SDA_PIN) || !defined(PICO_DEFAULT_I2C_SCL_PIN)
#warning i2c/lcd_1602_i2c example requires a board with I2C pins
#else
    // Initiera I2C och LCD
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    lcd_init();
    lcd_clear(); // Rensa skärmen vid start

    char distance_str[20]; // Buffert för att lagra distanssträngen
    char alarm_str[30];    // Buffert för att lagra larmsträngen

    while (true) {
        float distance = get_distance();
        //printf("Distance: %.2f cm\n", distance);

        if (distance < ALARM_THRESHOLD) {
            gpio_put(LED_PIN, 1);
        	lcd_set_cursor(0, 0);
		lcd_string("                ");

	    snprintf(alarm_str, sizeof(alarm_str), "---ALARM---");
            lcd_set_cursor(0, (MAX_CHARS - strlen(alarm_str)) / 2); // Centrera texten
            lcd_string(alarm_str);
	    
	    lcd_set_cursor(1,0);
		lcd_string("                ");
            snprintf(distance_str, sizeof(distance_str), "Distance: %.0f cm", distance);
            lcd_set_cursor(1, (MAX_CHARS - strlen(distance_str)) / 2); // Centrera texten
            lcd_string(distance_str);
            //printf("ALARM! Distance: %.2f cm\n", distance);
        } else {
            gpio_put(LED_PIN, 0);
            snprintf(distance_str, sizeof(distance_str), "Distance: %.0f cm", distance);
            lcd_set_cursor(0, (MAX_CHARS - strlen(distance_str)) / 2); // Centrera texten
            lcd_string(distance_str);
            lcd_set_cursor(1, 0); // Rensa andra raden
            lcd_string("                "); // Skriv över eventuellt larmmeddelande
        }

        sleep_ms(500);
    }
#endif

    return 0;
}
