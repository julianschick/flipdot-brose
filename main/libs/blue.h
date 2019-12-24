#ifndef BLUETOOTH_H_INCLUDED
#define BLUETOOTH_H_INCLUDED

#include "globals.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

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