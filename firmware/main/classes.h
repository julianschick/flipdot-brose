#ifndef FLIPDOT_CLASSES_H
#define FLIPDOT_CLASSES_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buffers/ws2812buffer.h"
extern WS2812Buffer* ledBuffer;

#include "buffers/flipdotbuffer.h"
extern FlipdotBuffer* flipdotBuffer;

extern TaskHandle_t ledTask;
extern TaskHandle_t tcpServerTask;
extern TaskHandle_t flipdotTask;

#endif //FLIPDOT_CLASSES_H
