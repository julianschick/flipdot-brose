#ifndef BLUETOOTH_H_INCLUDED
#define BLUETOOTH_H_INCLUDED

#include <string>
#include <esp_gap_bt_api.h>
#include <esp_spp_api.h>

#define SPP_SERVER_NAME "Flipdot/Serial"
#define BT_DEVICE_NAME "Flipdot1"

using namespace std;

class Blue {

public:
    static void setup();

private:
	static string buffer;
	static uint32_t client;
    
    static void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    static void security_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static void parse_buffer();

};

#endif //BLUETOOTH_H_INCLUDED