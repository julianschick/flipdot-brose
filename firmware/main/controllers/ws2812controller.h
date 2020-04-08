#ifndef WS2812CONTROLLER_H
#define WS2812CONTROLLER_H

#include <vector>
#include "../drivers/ws2812driver.h"

class WS2812Controller {

public:

    enum LedCommand {
        SET_ALL_LEDS,
        SET_LEDS,
        SET_LED_TRX_MODE
    };

    struct LedCommandMsg {
        LedCommand cmd;
        void* data;
    };

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

    void setTransitionMode(TransitionMode mode);
    void setAllLedsToSameColor(color_t color);
    void setLeds(std::vector<LedChangeCommand> *changes);

public:
    size_t getLedCount() { return n; };
    color_t* getAll();

    bool saveStateIfNecessary();
    void saveState();
    void readState();

    inline void setMutex(SemaphoreHandle_t m) { mutex = m; };
    inline void lock() {
        if (mutex != nullptr) {
            xSemaphoreTake(mutex, portMAX_DELAY);
        }
    }
    inline void unlock() {
        if (mutex != nullptr) {
            xSemaphoreGive(mutex);
        }
    }

private:
    WS2812Driver* drv;
    SemaphoreHandle_t mutex = nullptr;

    size_t n;
    color_t* state;

    TransitionMode transitionMode;
    color_t* startState;
    color_t* endState;
    int64_t lastCycle = 0;

    // for nvs storage
    bool newState = false;
    bool loadingFromNvs = false;

    // in milliseconds
    static const uint32_t LINEAR_SLOW_DELAY = 50;
    static const uint32_t LINEAR_MEDIUM_DELAY = 25;
    static const uint32_t LINEAR_QUICK_DELAY = 5;

    static const uint32_t SLIDE_SLOW_DELAY = 50;
    static const uint32_t SLIDE_MEDIUM_DELAY = 25;
    static const uint32_t SLIDE_QUICK_DELAY = 5;

    static int getDelayByMode(TransitionMode mode) {
        switch(mode) {
            case LINEAR_SLOW: return LINEAR_SLOW_DELAY;
            case LINEAR_MEDIUM: return LINEAR_MEDIUM_DELAY;
            case LINEAR_QUICK: return LINEAR_QUICK_DELAY;
            case SLIDE_SLOW: return SLIDE_SLOW_DELAY;
            case SLIDE_MEDIUM: return SLIDE_MEDIUM_DELAY;
            case SLIDE_QUICK: return SLIDE_QUICK_DELAY;
            default: return 0;
        }
    };

    void transition();
    void copy(color_t* dest, color_t* src);



};


#endif //WS2812CONTROLLER_H
