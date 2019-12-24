#ifndef WIFI_H_
#define WIFI_H_

#include "globals.h"

//#include <string.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/event_groups.h"
//#include "esp_system.h"
#include "esp_wifi.h"
//#include "esp_log.h"
//#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

using namespace std;

class Wifi {

public:
	static void setup();
	static string get_ip();

	static string get_ssid();
	static bool set_ssid(string ssid);
	static string get_pass();
	static bool set_pass(string pass);

	static bool enable();
	static bool disable();
	static bool is_enabled();
	static bool is_connected();

private:
	static EventGroupHandle_t wifi_status_bits;
	static const uint32_t SBIT_CONNECTED = 0x00000001;
	static const uint32_t SBIT_ACTIVE =    0x00000002;
	static const int max_retries = 10;
	static int number_of_retries;
	//
	static SemaphoreHandle_t ip_semaphore;
	static ip4_addr_t wifi_client_ip;
	static ip4_addr_t zero_ip;
	//
	static wifi_config_t config;
	static esp_timer_handle_t reconnect_timer;

	static void load_config();
	static bool commit_config();
	static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
	static void reconnect_timer_callback(void* arg);
};

#endif //WIFI_H_