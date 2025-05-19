#pragma once
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "mqtt_config.h"

/**
 * @brief All data related to mqtt connection
 */
typedef struct {
    mqtt_client_t *mqtt_client; ///< Native mqtt client object
    struct mqtt_connect_client_info_t mqtt_client_info; ///< Native client info
    ip_addr_t mqtt_server_address;                      ///< Broker address
    int mqtt_server_port;                               ///< Broker port
    char topic[MQTT_TOPIC_LEN];                         ///< Topic to publish to
    bool connect_done;      ///< Status of connection
    int published_messages; ///< Simple pay load used when publishing
    bool alarm_active;      ///< Track if alarm is currently active
} mqtt_client_data_t;
