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
#include <pico/time.h>
#include "wifi_credentials.h"
#include "hardware/i2c.h"
#include "lcd_printer.h"
#include "pico/binary_info.h"
#include "mqtt_config.h"
#include "sensor_config.h"
#include "mqtt_client_data.h"
#include "mqtt_functions.h"

//------------------MOVE TO HEADER LATER---------------------------
//#include <string.h>
//#include <time.h>
//#include "pico/stdlib.h"
//#include "pico/cyw43_arch.h"
//#include "lwip/dns.h"
//#include "lwip/pbuf.h"
//#include "lwip/udp.h"
//#include "wifi_credentials.h"
//#include "pico/time.h"
/*
typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    uint32_t ntp_test_time_ms_since_boot; 
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (5000)
#define NTP_RESEND_TIME (10 * 1000)

static NTP_T* ntp_state = NULL;

static void ntp_result(NTP_T* state, int status, time_t *result);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_request(NTP_T *state);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static NTP_T* ntp_init(void);
void ntp_periodic_check(void);
*/
/**
 * @brief Good old main
 */
int main(void) {
	//unplugging stabalizer
	sleep_ms(1000);      

      	stdio_init_all();
  	printf("Initializing Pico W...");
//sensor and lcd init
//
        // initilize GPIO-pins
        gpio_init(TRIG_PIN);
        gpio_set_dir(TRIG_PIN, GPIO_OUT);
        gpio_init(ECHO_PIN);
        gpio_set_dir(ECHO_PIN, GPIO_IN);
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
    
	
// Initiera I2C och LCD
#ifdef i2c_default
	i2c_init(i2c_default, 100 * 1000);
	gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
	gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    	lcd_init();
    	lcd_clear(); // clear screen at start
	lcd_string("Conn to WiFi..");
#endif

  if (cyw43_arch_init()) {
    panic("Failed to initialize CYW43");
  }

  printf("Attempting wifi connect...");
  cyw43_arch_enable_sta_mode();
  if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str(),
                                         CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    panic("Failed to Connect");
  }
#ifdef i2c_default
  	lcd_clear();
	lcd_string("WiFi connected!");
	lcd_set_cursor(1, 0);
	lcd_string("Conn to MQTT...");
	sleep_ms(1000);
#endif
  // Setup mqtt client info

  static mqtt_client_data_t state;
  state.alarm_active =false;
  if (!ipaddr_aton(_MQTT_BROKER_IP, &state.mqtt_server_address)) {
    panic("Could not convert broker IP");
  }
  state.mqtt_server_port = _MQTT_PORT;

  static const char* will_topic = "/alarm/offline";
  static const char* will_msg = "{offline}";

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
#ifdef i2c_default
	lcd_clear();
	lcd_string("MQTT Connected!");
	sleep_ms(1000);
	lcd_string("                     "); 
#endif
 //init NTP
 // ntp_state=ntp_init();

  // Main loop to wait for workers to do job
  while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client)) {
    cyw43_arch_poll();
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
  
   // ntp_periodic_check();
  }
  printf("Done, exiting...");
  /*if(ntp_state){
  	udp_remove(ntp_state->ntp_pcb); 
	free(ntp_state);
        ntp_state = NULL;
  }*/
  return 0;
}

//---------------------------------------------END OF MAIN------------------------------------------------------
//-------------------------------------------MOVE TO OWN CPP----------------------------------------------------
/*
// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result) {
    printf("NTP Result: %s", status == 0 ? "Success" : "Failed");
    if (status == 0 && result) {
        time_t local_time = *result + (2 * 60 * 60);
        struct tm *local_tm = gmtime(&local_time);
        printf(", Time: %02d/%02d/%04d %02d:%02d:%02d\n", local_tm->tm_mday, local_tm->tm_mon + 1, local_tm->tm_year + 1900,
               local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec);
    } else {
        printf("\n");
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    // Sätt nästa testtidpunkt som ms sedan boot
    state->ntp_test_time_ms_since_boot = to_ms_since_boot(get_absolute_time()) + NTP_TEST_TIME;
    state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
static void ntp_request(NTP_T *state) {
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
    printf("NTP Request sent...\n");
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T* state = (NTP_T*)user_data;
    printf("NTP request timed out.\n");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        ntp_request(state);
    } else {
        printf("NTP DNS lookup failed.\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        printf("Invalid NTP response received.\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static NTP_T* ntp_init(void) {
    NTP_T *state = (NTP_T*)calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("Failed to allocate NTP state.\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("Failed to create NTP UDP PCB.\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    // Lagra nästa testtidpunkt som ms sedan boot
    state->ntp_test_time_ms_since_boot = to_ms_since_boot(get_absolute_time()) + NTP_TEST_TIME;
    state->dns_request_sent = false;
    printf("NTP Initialized.\n");
    return state;
}

void ntp_periodic_check(void) {
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if (ntp_state) {
        if (now_ms >= ntp_state->ntp_test_time_ms_since_boot && !ntp_state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            ntp_state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, ntp_state, true);

            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &ntp_state->ntp_server_address, ntp_dns_found, ntp_state);
            cyw43_arch_lwip_end();

            ntp_state->dns_request_sent = true;
            if (err == ERR_OK) {
                // Do nothing here, ntp_dns_found will be called
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("NTP DNS lookup error: %d\n", err);
                ntp_result(ntp_state, -1, NULL);
            } else {
                printf("NTP DNS lookup in progress...\n");
            }
        }
#if PICO_CYW43_ARCH_POLL
        cyw43_arch_poll();
#endif
    }
}*/
