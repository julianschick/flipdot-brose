#ifndef FLIPDOT_CLASSES_H
#define FLIPDOT_CLASSES_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "controllers/flipdotdisplay.h"
extern FlipdotDisplay* dsp;

#include "controllers/ws2812controller.h"
extern WS2812Controller* led_ctrl;

#include "buffers/ws2812buffer.h"
extern WS2812Buffer* ledBuffer;

extern TaskHandle_t ledTask;
extern TaskHandle_t tcpServerTask;

#endif //FLIPDOT_CLASSES_H
