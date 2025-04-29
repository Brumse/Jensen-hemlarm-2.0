#pragma once
#include "mqtt_client_data.h"



void start_client(mqtt_client_data_t *state);
void mqtt_connection_cb(mqtt_client_t *client, void *arg,
                               mqtt_connection_status_t status);
void pub_request_cb(void *arg, err_t err);
void publish_worker_fn(async_context_t *context,
                              async_at_time_worker_t *worker);


