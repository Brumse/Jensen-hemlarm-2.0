#include <stdio.h>
#include <string>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio-h"
#include "hardware/timer.h"
#include "wifi_credentials.h"
#include "MQTTClient.h"


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
const float ALARM_THREHOLD = 10.0f;

//MQTT client
MQTTCLient client;
Network network;
unsigned char sendbuf[256];
unsigned char readbuf[256];


void trigger_pulse()
{
	gpio_put(TRIG_PIN, 1);
	sleep_us(10);
	pgio_put(TRIG_PIN, 0);
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

void mqtt_connect() {
    NetworkInit(&network);
    MQTTClientInit(&client, &network, 1000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "pico_motion_sensor";

    printf("Connecting to MQTT broker...\n");
    if(NetworkConnect(&network, mqtt_broker_ip, mqtt_broker_port) < 0 ||
       MQTTConnect(&client, &data) < 0) {
        printf("MQTT connection failed!\n");
        return;
    }
    printf("MQTT connected!\n");
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
	mqtt_connect();
	
	 while(true) {
        float distance = get_distance();

        // Publish to MQTT
        char message[50];
        sprintf(message, "%.2f", distance);
        MQTTMessage mqtt_msg = {
            .qos = QOS0,
            .retained = 0,
            .payload = message,
            .payloadlen = strlen(message)
        };

        if(MQTTPublish(&client, mqtt_topic, &mqtt_msg) < 0) {
            printf("Publish failed, reconnecting...\n");
            mqtt_connect();
        }

        // Check alarm threshold
        if(distance < ALARM_THRESHOLD) {
            gpio_put(LED_PIN, 1);
            printf("ALARM! Distance: %.2f cm\n", distance);
        } else {
            gpio_put(LED_PIN, 0);
        }

        sleep_ms(500);
    }

    cyw43_arch_deinit();

	return 0;
}
