#include "config.h"
#include <ctime>
#include <curl/curl.h>
#include <jansson.h>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb,
                                  void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "not enough memory (realloc return NULL)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

char *build_json_payload(const char *device_id, const char *key,
                         const char *value) {
    json_t *root = json_object();
    json_object_set_new(root, "device_id", json_string(device_id));
    json_object_set_new(root, key, json_string(value));
    char *payload = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return payload;
}

char *build_motion_json_payload(const char *device_id, double distance) {
    json_t *root = json_object();
    json_object_set_new(root, "device_id", json_string(device_id));
    json_object_set_new(root, "distance", json_real(distance));
    char *payload = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return payload;
}

CURLcode send_http_post(const char *url, const char *payload,
                        struct MemoryStruct *chunk) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(payload));
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return res;
}

void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    if (rc == MOSQ_ERR_SUCCESS) {
        printf("Connected to MQTT Broker!\n");
        mosquitto_subscribe(mosq, NULL, MQTT_TOPIC, 0);
        mosquitto_subscribe(mosq, NULL, MQTT_STATUS_TOPIC, 0);
    } else {
        fprintf(stderr, "failed to connect to MQTT Broker, returncode %d\n",
                rc);
    }
}
void on_message(struct mosquitto *mosq, void *userdata,
                const struct mosquitto_message *msg) {
    if (msg->payloadlen > 0) {
        printf("Message received from %s: %s\n", msg->topic,
               (char *)msg->payload);

        if (strcmp(msg->topic, "/alarm/status") == 0) {
            json_error_t error;
            json_t *will_root = json_loads((char *)msg->payload, 0, &error);
            if (!will_root) {
                fprintf(stderr, "error parsing of Will JSON: %s\n", error.text);
                return;
            }

            json_t *device_id_json = json_object_get(will_root, "device_id");
            json_t *name_json = json_object_get(will_root, "name");
            json_t *status_json = json_object_get(will_root, "status");

            const char *device_id = json_string_value(device_id_json);
            const char *name = json_string_value(name_json);
            const char *status = json_string_value(status_json);

            printf(
                "Will Message Received - Device ID: %s, Name: %s, Status: %s\n",
                device_id, name, status);

            char api_url[256];
            snprintf(api_url, sizeof(api_url), "%s/device_status",
                     API_BASE_URL);

            char *will_payload = json_dumps(will_root, JSON_COMPACT);
            printf("Sending Will payload to API: %s\n", will_payload);

            struct MemoryStruct chunk;
            chunk.memory = (char *)malloc(1);
            chunk.size = 0;
            CURLcode res = send_http_post(api_url, will_payload, &chunk);

            if (res == CURLE_OK) {
                long response_code;
                curl_easy_getinfo(curl_easy_init(), CURLINFO_RESPONSE_CODE,
                                  &response_code);
                printf("Will status sent. Status code: %ld, Response: %s\n",
                       response_code, chunk.memory);
            } else {
                fprintf(stderr, "Failed to send Will status: %s\n",
                        curl_easy_strerror(res));
            }

            free(will_payload);
            free(chunk.memory);
            json_decref(will_root);

            return;
        }

        json_error_t error;
        json_t *root = json_loads((char *)msg->payload, 0, &error);

        if (!root) {
            fprintf(stderr, "error parsing of JSON: %s\n", error.text);
            return;
        }

        json_t *device = json_object_get(root, "device");
        json_t *alarm_active = json_object_get(root, "alarm_active");
        json_t *distance = json_object_get(root, "distance");

        const char *device_str = json_string_value(device);
        int alarm_int = json_boolean_value(alarm_active);
        double distance_val = json_real_value(distance);

        printf("Device: %s, Alarm Active: %d, Distance: %.2f\n", device_str,
               alarm_int, distance_val);

        char motion_url[256];
        snprintf(motion_url, sizeof(motion_url), "%s/motion_detected",
                 API_BASE_URL);

        json_t *motion_data = json_object();
        json_object_set_new(motion_data, "device_id", json_string(device_str));
        json_object_set_new(motion_data, "distance", json_real(distance_val));
        json_object_set_new(motion_data, "alarm_active",
                            json_boolean(alarm_int)); // alarm_active

        char *payload = json_dumps(motion_data, JSON_COMPACT);
        printf("Motion data payload sent: %s\n",
               payload); // logg payloaden sent

        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        CURLcode res = send_http_post(motion_url, payload, &chunk);

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl_easy_init(), CURLINFO_RESPONSE_CODE,
                              &response_code);
            printf("Motion data sent. Status code: %ld, Response: %s\n",
                   response_code, chunk.memory);
        } else {
            fprintf(stderr, "Failed to send motion data: %s\n",
                    curl_easy_strerror(res));
        }

        free(payload);
        free(chunk.memory);
        json_decref(motion_data);
        json_decref(root);
    }
}
int main() {
    mosquitto_lib_init();
    time_t timestamp;
    time(&timestamp);
    printf("Current time is: %s", ctime(&timestamp));

    struct mosquitto *mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "couldn't create a mosquitto-instance\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    if (mosquitto_connect(mosq, MQTT_BROKER_IP, MQTT_PORT, 60) !=
        MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "couldn't connect to broker.\n");
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
