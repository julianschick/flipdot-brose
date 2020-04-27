#ifndef FLIPDOTBUFFER_H
#define FLIPDOTBUFFER_H

#include <vector>
#include <freertos/FreeRTOS.h>
//#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "../controllers/flipdotdisplay.h"

class FlipdotBuffer {

private:
    enum FlipdotCommand {
        SHOW_BITSET,
        CLEAR,
        FILL,
        SHOW_TEXT,
        SET_PIXELS,
        SET_DISPLAY_MODE,
        SET_FLIP_SPEED,
        SET_TRX_MODE
    };

    struct FlipdotCommandMsg {
        FlipdotCommand cmd;
        void* data;
    };

    struct ShowTextCommand {
        std::string text;
        PixelString::TextAlignment alignment;

        ShowTextCommand(const string &text, PixelString::TextAlignment alignment) : text(text), alignment(alignment) { };
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
    bool showText(std::string text, PixelString::TextAlignment alignment);
    bool setPixel(int x, int y, bool visible);
    bool setDisplayMode(FlipdotDisplay::DisplayMode displayMode);
    bool setFlipSpeed(uint16_t pixelsPerSecond);
    bool setTransitionMode(FlipdotDisplay::TransitionMode trxMode);

    int getNumberOfPixels() { return ctrl->get_number_of_pixels(); };
    bool isPixelValid(int x, int y) { return ctrl->is_valid_index(x, y); };
    bool isStateKnown();
    bool getPixel(int x, int y);
    size_t getBitset(uint8_t* buffer, size_t len);

    bool executeNext(int timeout);

private:
    FlipdotDisplay* ctrl;

    QueueHandle_t queue;
    SemaphoreHandle_t mutex;

    FlipdotCommandMsg msg;

    void executeCommand(FlipdotCommandMsg& msg);

};


#endif //FLIPDOTBUFFER_H
