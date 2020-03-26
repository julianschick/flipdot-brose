#ifndef GLOBALS_H_
#define GLOBALS_H_

// Basis
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdint.h>
#include "sys/param.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

// Log Tags
#define TAG_BT "bluetooth"
#define TAG_HTTP "http"
#define TAG_NVS "nvs"

#define MDNS_HOSTNAME "flipdot1"
#define MDNS_INSTANCE_NAME "Flipdot Controller"
#define MDNS_SERVICE_TYPE "_flipdot-tcp"
#define MDNS_SERVICE_NAME "Flipdot Controller TCP Interface"

#define SPP_SERVER_NAME "Flipdot/Serial"
#define BT_DEVICE_NAME "Flipdot1"
#define DHCP_HOSTNAME "flipdot1"

// Syntactic Sugar
#define PRIVATE_SYMBOLS namespace {
#define PRIVATE_END }

#endif //GLOBALS_H_