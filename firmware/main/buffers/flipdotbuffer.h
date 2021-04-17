#ifndef FLIPDOTBUFFER_H
#define FLIPDOTBUFFER_H

#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../controllers/flipdotdisplay.h"

class FlipdotBuffer {

private:
    enum FlipdotCommand {
        SHOW_BITSET,
        CLEAR,
        FILL,
        SET_PIXELS,
        SCROLL,
        SET_DISPLAY_MODE,
        SET_FLIP_SPEED,
        SET_TRX_MODE
    };

    struct FlipdotCommandMsg {
        FlipdotCommand cmd;
        void* data;
    };

    struct SetPixelCommand {
        int x, y;
        bool visible;

        SetPixelCommand(int x, int y, bool visible) : x(x), y(y), visible(visible) { };
    };


public:
    FlipdotBuffer(FlipdotDisplay* ctrl, size_t queueLength);
    ~FlipdotBuffer();

    bool showBitset(BitArray* bitset);
    bool clear();
    bool fill();
    bool setPixel(int x, int y, bool visible);
    bool scroll(std::vector<uint16_t>* dataAndMask);
    bool setDisplayMode(FlipdotDisplay::DisplayMode displayMode);
    bool setFlipSpeed(uint16_t pixelsPerSecond);
    bool setTransitionMode(FlipdotDisplay::TransitionMode trxMode);

    size_t getBitset(uint8_t* buffer, size_t len);
    int getDisplayHeight() { return ctrl->get_height(); };
    int getDisplayWidth() { return ctrl->get_width(); };
    int getNumberOfPixels() { return ctrl->get_number_of_pixels(); };
    bool isPixelValid(int x, int y) { return ctrl->is_valid_index(x, y); };
    bool isStateKnown();
    bool getPixel(int x, int y);
    FlipdotDisplay::DisplayMode getDisplayMode();
    uint16_t getFlipSpeed();
    FlipdotDisplay::TransitionMode getTransitionMode();


    bool executeNext(int timeout);

private:
    FlipdotDisplay* ctrl;

    QueueHandle_t queue;
    SemaphoreHandle_t mutex;

    FlipdotCommandMsg msg;

    void executeCommand(FlipdotCommandMsg& msg);

};


#endif //FLIPDOTBUFFER_H
