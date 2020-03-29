#ifndef FLIPDOT_CLASSES_H
#define FLIPDOT_CLASSES_H

#include "flipdotdisplay.h"
extern FlipdotDisplay* dsp;

#include "drivers/ws2812driver.h"
extern WS2812Driver* led_drv;

#include "controllers/ws2812controller.h"
extern WS2812Controller* led_ctrl;

#endif //FLIPDOT_CLASSES_H
