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
#include <string.h>
#include <time.h>
//#include "pico/stdlib.h"
//#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
//#include "wifi_credentials.h"


typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

static void ntp_result(NTP_T* state, int status, time_t *result);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_request(NTP_T *state);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static NTP_T* ntp_init(void);
void run_ntp_test(void);


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
    	lcd_clear(); // Rensa skärmen vid start
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
#ifdef i2c_default
	lcd_clear();
	lcd_string("MQTT Connected!");
	sleep_ms(1000);
	lcd_string("                     "); 
#endif


  // Main loop to wait for workers to do job
  while (!state.connect_done || mqtt_client_is_connected(state.mqtt_client)) {
    cyw43_arch_poll();
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(10000));
  
    run_ntp_test();
  }
  printf("Done, exiting...");
  return 0;
}

//---------------------------------------------END OF MAIN------------------------------------------------------
//----------------------------------------------MOVE TO OWN CPP------------------------------------------------

// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result) {
    if (status == 0 && result) {
        time_t local_time = *result + (2 * 60 * 60);
        struct tm *local_tm = gmtime(&local_time);
        printf("got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", local_tm->tm_mday, local_tm->tm_mon + 1, local_tm->tm_year + 1900,
               local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec);
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T* state = (NTP_T*)user_data;
    printf("ntp request failed\n");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("ntp dns request failed\n");
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
        printf("invalid ntp response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static NTP_T* ntp_init(void) {
    NTP_T *state = (NTP_T*)calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

// Runs ntp test forever
void run_ntp_test(void) {
    NTP_T *state = ntp_init();
    if (!state)
        return;
    while(true) {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
            // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
            // these calls are a no-op and can be omitted, but it is a good practice to use them in
            // case you switch the cyw43_arch type later.
            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(state); // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("dns request failed\n");
                ntp_result(state, -1, NULL);
            }
        }
#if PICO_CYW43_ARCH_POLL
        // if you are using pico_cyw43_arch_poll, then you must poll periodically from your
        // main loop (not from a timer interrupt) to check for Wi-Fi driver or lwIP work that needs to be done.
        cyw43_arch_poll();
        // you can poll as often as you like, however if you have nothing else to do you can
        // choose to sleep until either a specified time, or cyw43_arch_poll() has work to do:
        cyw43_arch_wait_for_work_until(state->dns_request_sent ? at_the_end_of_time : state->ntp_test_time);
#else
        // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
        // is done via interrupt in the background. This sleep is just an example of some (blocking)
        // work you might be doing.
        sleep_ms(1000);
#endif
    }
    free(state);
}

