#ifndef WS2812BUFFER_H
#define WS2812BUFFER_H

#include <vector>
#include "../controllers/ws2812controller.h"

class WS2812Buffer {

private:
    enum LedCommand {
        SET_ALL_LEDS,
        SET_LEDS,
        SET_LED_TRX_MODE
    };

    struct LedCommandMsg {
        LedCommand cmd;
        void* data;
    };

public:
    WS2812Buffer(WS2812Controller* ctrl, size_t queueLength);
    ~WS2812Buffer();

    bool setTransitionMode(WS2812Controller::TransitionMode mode);
    bool setAllLedsToSameColor(color_t color);
    bool setLeds(std::vector<WS2812Controller::LedChangeCommand> *changes);

    color_t* getAll();
    size_t getLedCount() { return ctrl-> getLedCount(); };
    WS2812Controller::TransitionMode getTransitionMode();

    bool executeNext(int timeout);

private:
    WS2812Controller* ctrl;

    QueueHandle_t queue;
    SemaphoreHandle_t mutex;

    LedCommandMsg msg;

    void executeCommand(LedCommandMsg& msg);

};


#endif //FLIPDOT_WS2812BUFFER_H
