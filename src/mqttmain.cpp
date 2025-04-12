/**
 * @file mqttmain.cpp
 * @brief Main file for Pico W MQTT application
 *
 * Freely adapted from
 * [mqtt.c](https://github.com/raspberrypi/pico-examples/blob/master/pico_w/wifi/mqtt/mqtt_client.c)
 */
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


#define _MQTT_BROKER_IP "192.168.1.140"
#define _MQTT_PORT 1883
#define MQTT_TOPIC_LEN 100
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 2 //changed to QOS 2 to make sure we get all messages
#define MQTT_PUBLISH_RETAIN 0

/*
#define MQTT_WILL_TOPIC "/online"
#define MQTT_WILL_MSG "0"
#define MQTT_WILL_QOS 1
*/
#ifndef MQTT_DEVICE_NAME
#define MQTT_DEVICE_NAME "pico"
#endif

// Custom struct definitions

/**
 * @brief All data related to mqtt connection
 */
typedef struct {
  mqtt_client_t *mqtt_client; ///< Native mqtt client object
  struct mqtt_connect_client_info_t mqtt_client_info; ///< Native client info
  ip_addr_t mqtt_server_address;                      ///< Broker address
  int mqtt_server_port;                               ///< Broker port
  char topic[MQTT_TOPIC_LEN];                         ///< Topic to publish to
  bool connect_done;                                  ///< Status of connection
  int published_messages; ///< Simple pay load used when publishing
} mqtt_client_data_t;

// Forward declarations
static void start_client(mqtt_client_data_t *state);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
                               mqtt_connection_status_t status);
static void pub_request_cb(void *arg, err_t err);
static void publish_worker_fn(async_context_t *context,
                              async_at_time_worker_t *worker);

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
static void start_client(mqtt_client_data_t *state) {
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
static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
                               mqtt_connection_status_t status) {
  mqtt_client_data_t *state = (mqtt_client_data_t *)arg; // Cast user data
  if (status == MQTT_CONNECT_ACCEPTED) {
    state->connect_done = true;
    state->published_messages = 0;

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
static void pub_request_cb(void *arg, err_t err) {
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
static void publish_worker_fn(async_context_t *context,
                              async_at_time_worker_t *worker) {
  mqtt_client_data_t *state = (mqtt_client_data_t *)worker->user_data;
  char msg[32] ={0};
  snprintf(msg, sizeof(msg), "Msgs: %d", ++(state->published_messages));
  mqtt_publish(state->mqtt_client, state->topic, msg, strlen(msg),
               MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, state);

  async_context_add_at_time_worker_in_ms(context, worker, 1000);
}

/**
 * @brief Good old main
 */
int main(void) {
      
  stdio_init_all();

  //adding delay yo connect to minicom..
  sleep_ms(3000);
  printf("Initializing Pico W...");

  if (cyw43_arch_init()) {
    panic("Failed to initialize CYW43");
  }

  printf("Attempting wifi connect...");
  cyw43_arch_enable_sta_mode();
  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(),
                                         CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    panic("Failed to Connect");
  }

  // Setup mqtt client info

  static mqtt_client_data_t state;
  if (!ipaddr_aton(_MQTT_BROKER_IP, &state.mqtt_server_address)) {
    panic("Could not convert broker IP");
  }
  state.mqtt_server_port = _MQTT_PORT;
  strcpy(state.topic, "/messagepub");


  static const char* will_topic = "/alarm/offline";
  static const char* will_msg = "pico w Offline!!";

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

  // Main loop to wait for workers to do job
  while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client)) {
    cyw43_arch_poll();
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
  }

  printf("Done, exiting...");
  return 0;
}
