#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "wifi_credentials.h"
#include "pico_mqtt.h"
#include "hardware/timer.h"

/*
NOTES for adding functions:

	add LCD-display to show text when connected to wifi and mqtt and also MAYBE indicate trigger alarm.
	add arlarmsound when trigger alarm.
	add button to reset alarm.
*/

const uint TRIG_PIN = 2;
const uint ECHO_PIN = 3;
const uint LED_PIN = 15;

// move to header?
const char *mqtt_broker_ip = 192.168.1.117;
const int mqtt_broker_port = 1883;
const char *mqtt_topic = "distance/sensor";

// set threshold for alarm
const float ALARM_THREHOLD = 50;

void trigger_pulse()
{
	gpio_put(TRIG_PIN, 1);
	sleep_us(10);
	pgio_put(TRIG_PIN, 0);
}

float get_distance()
{
	uint32_t start_time = 0;
	uint32_t end_time = 0;

	trigger_pulse();
	while (gpio_get(ECHO_PIN) == 0)
		;
	start_time = time_us_32();

	while (gpio_get(ECHO_PIN) == 1)
		;
	end_time = time_us_32();

	uint32_t pulse_duration = end_time - start_time;
	// magic number !!??
	float distance = pulse_duration * 0.0343 / 2;

	return distance;
}

int main()
{
	stdio_init_all();

	if (cyw43_arch_init())
	{
		printf("failed to initialise\n");
		return 1;
	}
	printf("initialised\n");

	cyw43_arch_enable_sta_mode();

	printf("Connecting to WiFi...\n");
	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(), CYW43_AUTH_WPA2_AES_PSK, 30000))
	{
		printf("failed to connect\n");
		cyw43_arch_deinit();
		return 1;
	}
	else
	{
		printf("Connected to WiFi\n");
	}

	// initilize GPIO-pins
	gpio_init(TRIG_PIN);
	gpio_set_dir(TRIG_PIN, GPIO_OUT);
	gpio_init(ECHO_PIN);
	gpio_set_dir(ECHO_PIN, GPIO_IN);
	gpio_init(LED_PIN);
	gpio_set_dir(LED_PIN, GPIO_OUT);

	// mqtt logic & connection
	mqtt_client *client = mqtt_client_new();
	if (mqtt_client_connect(client, mqtt_broker_ip, mqtt_broker_port))
	{
		printf("Connected to MQTT broker/n");
	}
	else
	{
		printf("Failed to connect to MQTT broker/n");
		return 1;
	}

	while (true)
	{
		float distance = get_distance();

		if (distance < ALARM_THRESHOLD)
		{
			gpio_put(LED_PIN, 1);
			printf("ALARM! DIstance below threshold.\n");
		}
		else
		{
			gpio_put(LED_PIN, 0);
		}

		char message[50];
		sprintf(message, "%.2f", distance);
		mqtt_publish(client, mqtt_topic, message);
		sleep_ms(1000);
	}

	mqtt_client_disconnect(client);
	mqtt_client_free(client);

	return 0;
}
