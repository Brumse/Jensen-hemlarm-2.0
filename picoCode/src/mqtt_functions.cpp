#include "mqtt_functions.h"
#include "hardware/i2c.h"
#include "lcd_printer.h"
#include "lwip/apps/mqtt.h"
#include "mqtt_client_data.h"
#include "mqtt_config.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "sensor_config.h"
#include "sensor_functions.h"
#include "wifi_credentials.h"
#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <pico/async_context.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include "pico/time.h"
// Global objects
/**
 * @brief Worker object that does asynchronous work
 */
static async_at_time_worker_t publish_worker = {.do_work = publish_worker_fn};

// Function definitions
/**
 * @brief Setup client and start workers
 * @param state Pointer to the state struct
 */
void start_client(mqtt_client_data_t *state) {
    state->mqtt_client = mqtt_client_new();
    if (!state->mqtt_client) {
        panic("MQTT client instance creation error");
    }

    printf("IP of device: %s\n", ipaddr_ntoa(&netif_list->ip_addr));
    printf("Connecting to MQTT broker at %s\n",
           ipaddr_ntoa(&state->mqtt_server_address));

    // Protected reqion to access raw LwIP functions
    cyw43_arch_lwip_begin();
    if (mqtt_client_connect(state->mqtt_client, &state->mqtt_server_address,
                            state->mqtt_server_port, mqtt_connection_cb, state,
                            &state->mqtt_client_info) != ERR_OK) {
        panic("MQTT broker connection error");
    }
    cyw43_arch_lwip_end();
}

/**
 * @brief Connection callback
 * @param client Native mqtt client
 * @param void User supplied arg. Here the mqtt_client_data_t
 * @param status Native mqtt connection status
 */
void mqtt_connection_cb(mqtt_client_t *client, void *arg,
                        mqtt_connection_status_t status) {
    mqtt_client_data_t *state = (mqtt_client_data_t *)arg; // Cast user data
    if (status == MQTT_CONNECT_ACCEPTED) {

        printf("MQTT connection failed\n"); // debugging print
        state->connect_done = true;
        state->published_messages = 0;

        const char *online_topic = "/alarm/status"; // online subject
        char online_msg_buffer[128];
        snprintf(
            online_msg_buffer, sizeof(online_msg_buffer),
            "{\"device_id\": \"%s\", \"name\": \"%s\", \"status\": \"online\"}",
            MQTT_DEVICE_NAME, MQTT_DEVICE_NAME);
        char *online_msg = online_msg_buffer; // online message
        err_t err;

        printf("Trying to publish online-status to %s \n", online_topic);

        cyw43_arch_lwip_begin();
        err = mqtt_publish(state->mqtt_client, // MQTT-Client
                           online_topic,       // subject
                           online_msg,         // Message-payload
                           strlen(online_msg), // length of payload
                           MQTT_PUBLISH_QOS,   // QoS
                           0,                  // Retain flag (0 = send now)
                           pub_request_cb,     // Callback confirmation
                           state);             // Argument to callback
        cyw43_arch_lwip_end();

        if (err != ERR_OK) {
            printf("Failed to publish online-status. Errorcode: %d\n", err);
        } else {
            printf("Online-status published!\n");
        }

        // Start publish worker
        publish_worker.user_data = state;
        async_context_add_at_time_worker_in_ms(cyw43_arch_async_context(),
                                               &publish_worker, 0);

    } else if (status == MQTT_CONNECT_DISCONNECTED) {
        if (state->connect_done) {
            panic("Failed to connect to MQTT server");
        }
    } else {
        panic("Unexpected state");
    }
}

/**
 * @brief Publish request callback
 * @param arg User supplied argument
 * @param Native error type
 */
void pub_request_cb(void *arg, err_t err) {
    if (err != 0) {
        printf("pub_request_cb failed %d", err);
    }
}

/**
 * @brief The actual work the worker does
 * @param context Native async context
 * @param worker Native async worker
 *
 * Based on native async worker object. This is where the actual work happens.
 */
void publish_worker_fn(async_context_t *context,
                       async_at_time_worker_t *worker) {
    mqtt_client_data_t *state = (mqtt_client_data_t *)worker->user_data;
    char msg[125] = {0};
    float distance = get_distance();
    bool current_distance_alarm  = (distance < ALARM_THRESHOLD);
    static bool alarm_active_latch = false;
    static uint32_t last_button_press_time = 0;
    static bool alarm_cleared = false;
    static uint32_t cleared_message_end_time = 0;
    const uint32_t CLEAR_MESSAGE_DURATION_MS = 2000;
    
    bool clear_button_down = !gpio_get(CLEAR_BUTTON_PIN);
    uint32_t current_time = time_us_32();

    // Debouncing and reset alarm
    if (clear_button_down && (current_time - last_button_press_time > 200000)) {
        alarm_active_latch = false;
        alarm_cleared = true;
        cleared_message_end_time = current_time + CLEAR_MESSAGE_DURATION_MS * 1000;
        last_button_press_time = current_time;
    } else if (current_distance_alarm) {
        alarm_active_latch = true;
        alarm_cleared = false; 
    }

#ifdef i2c_default
    char distance_str[20];
    snprintf(distance_str, sizeof(distance_str), "Distance: %.0f cm", distance);
    lcd_set_cursor(0, 0);
    lcd_string(distance_str);

    // Visa larmstatus
    lcd_set_cursor(1, 0);
    if (alarm_cleared && current_time < cleared_message_end_time) {
        lcd_string("---CLEARED---");
    } else {
        lcd_string(alarm_active_latch ? "---ALARM---" : "             ");
        if (!alarm_active_latch) {
            alarm_cleared = false;
        }
    }
#endif

    gpio_put(LED_PIN, alarm_active_latch);

    // Publish to MQTT if alarm is triggered
    if (alarm_active_latch != state->alarm_active) {

        state->alarm_active = alarm_active_latch;

        snprintf(
            msg, sizeof(msg),
            "{\"device\": \"%s\", \"alarm_active\": %s, \"distance\": %.2f}",
            MQTT_DEVICE_NAME, state->alarm_active ? "true" : "false", distance);
        mqtt_publish(state->mqtt_client, "/motion/distance", msg, strlen(msg),
                     MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb,
                     state);
    }
    async_context_add_at_time_worker_in_ms(context, worker, 1000);
}
