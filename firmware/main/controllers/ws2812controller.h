#ifndef WS2812CONTROLLER_H
#define WS2812CONTROLLER_H

#include <vector>
#include "../drivers/ws2812driver.h"

class WS2812Controller {

private:
    enum LedCommand {
        SET_ALL_LEDS,
        SET_LEDS,
        GET_ALL_LEDS,
        SET_LED_TRX_MODE
    };

    struct LedCommandMsg {
        LedCommand cmd;
        void* data;
    };

public:
    struct LedChangeCommand {
        size_t addr;
        color_t color;
    };

    enum TransitionMode {
        IMMEDIATE           = 0x01,
        LINEAR_SLOW         = 0x02,
        LINEAR_MEDIUM       = 0x03,
        LINEAR_QUICK        = 0x04,
        SLIDE_SLOW          = 0x05,
        SLIDE_MEDIUM        = 0x06,
        SLIDE_QUICK         = 0x07
    };

    WS2812Controller(WS2812Driver* drv);
    ~WS2812Controller();

    bool setTransitionMode(TransitionMode mode);
    bool setAllLedsToSameColor(color_t color);
    bool setLeds(std::vector<LedChangeCommand> *changes);

private:
    void setTransitionMode_(TransitionMode* mode);
    void setAllLedsToSameColor_(color_t* c);
    void setLeds_(std::vector<LedChangeCommand> *changes);

public:
    size_t getLedCount() { return size; };
    color_t* getAll();

    void cycle();
    bool isBusy();

    void saveState();
    void readState();

private:
    WS2812Driver* drv;

    size_t size;
    color_t* state;

    QueueHandle_t queue;
    SemaphoreHandle_t mutex;

    TransitionMode transitionMode;
    color_t* startState;
    color_t* endState;
    uint8_t transitionPosition = 0;
    bool inTransition = false;
    int64_t lastCycle = 0;

    // for nvs storage
    bool newState = false;
    bool loadingFromNvs = false;
    TickType_t lastStateSave = 0;

    // in milliseconds
    static const uint32_t LINEAR_SLOW_DELAY = 50;
    static const uint32_t LINEAR_MEDIUM_DELAY = 25;
    static const uint32_t LINEAR_QUICK_DELAY = 5;

    static int getDelayByMode(TransitionMode mode) {
        switch(mode) {
            case LINEAR_SLOW: return LINEAR_SLOW_DELAY;
            case LINEAR_MEDIUM: return LINEAR_MEDIUM_DELAY;
            case LINEAR_QUICK: return LINEAR_QUICK_DELAY;
            default: return 0;
        }
    };

    void startTransition();
    void copy(color_t* dest, color_t* src);
    void executeCommand(LedCommandMsg& msg);



};


#endif //WS2812CONTROLLER_H
