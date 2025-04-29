#include "lwip/apps/mqtt.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <lwip/ip4_addr.h>
#include <lwip/netif.h>
#include <pico/async_context.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include "wifi_credentials.h"
#include "hardware/i2c.h"
#include "lcd_printer.h"
#include "pico/binary_info.h"
#include "mqtt_config.h"
#include "sensor_functions.h"
#include "sensor_config.h"
#include "mqtt_client_data.h"
#include "mqtt_functions.h"



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

	  printf("MQTT connection failed\n");//debugging print
    state->connect_done = true;
    state->published_messages = 0;


        const char* online_topic = "/alarm/status"; //online subject
        const char* online_msg = "{online}";  // online message
        err_t err;

	printf("Trying to publish online-status to %s \n", online_topic);

	        cyw43_arch_lwip_begin();
        err = mqtt_publish(state->mqtt_client,      // MQTT-Client
                           online_topic,            // subject
                           online_msg,              // Message-payload
                           strlen(online_msg),      // length of payload
                           MQTT_PUBLISH_QOS,        // QoS
                           0,                       // Retain flag (0 = send now)
                           pub_request_cb,          // Callback confirmation
                           state);                  // Argument to callback
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
  char msg[32] ={0};
  float distance = get_distance();
  bool alarm_triggered = (distance < ALARM_THRESHOLD);

      #ifdef i2c_default
        char distance_str[20];
        char alarm_str[30];

        //clear screen
        lcd_set_cursor(0, 0);
        lcd_string("                ");


        snprintf(distance_str, sizeof(distance_str), "Distance: %.0f cm", distance);
        lcd_set_cursor(0,0);
        lcd_string(distance_str);

        //clear screen
        lcd_set_cursor(1, 0);
        lcd_string("                ");

        lcd_set_cursor(1,0);
        lcd_string(alarm_triggered ? "---ALARM---" : "             ");
    #endif

        gpio_put(LED_PIN,alarm_triggered);

     // Publish to MQTT if alarm is triggered
    if (alarm_triggered != state->alarm_active) {
        if(alarm_triggered){
	  snprintf(msg, sizeof(msg), "Distance : %.2f", distance);
          mqtt_publish(state->mqtt_client, "/motion/distance", msg, strlen(msg),
                       MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

        snprintf(msg, sizeof(msg), "--Alarm-- ");
    }else{
	snprintf(msg, sizeof(msg),"--Alarm cleared--");
    }

	mqtt_publish(state->mqtt_client, "/motion/alarm", msg, strlen(msg),
                     MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);
    state->alarm_active =alarm_triggered;
    }
         async_context_add_at_time_worker_in_ms(context, worker, 1000);

}
