#ifndef FLIPDOTDISPLAY_H
#define FLIPDOTDISPLAY_H

#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../util/bitarray.h"
#include "../util/pixelmap.h"
#include "../util/pixelstring.h"
#include "../drivers/flipdotdriver.h"

#define LEFT_TO_RIGHT_COORD(c) c.x = i / h; c.y = i % h
#define RIGHT_TO_LEFT_COORD(c) c.x = w - 1 - (i / h); c.y = i % h
#define RANDOM_COORD(c) c.x = randomTransitionMap[i].x; c.y = randomTransitionMap[i].y
#define TOP_DOWN_COORD(c) c.x = i % w; c.y = i / w
#define BOTTOM_UP_COORD(c) c.x = i % w; c.y = h - 1 - (i / w)


class FlipdotDisplay {

public:
    enum DisplayMode {
        OVERRIDE        = 0x00,
        INCREMENTAL     = 0x01
    };
    static const DisplayMode DisplayMode_Last = INCREMENTAL;

    enum TransitionMode {
        LEFT_TO_RIGHT   = 0x01,
        RIGHT_TO_LEFT   = 0x02,
        RANDOM          = 0x03,
        TOP_DOWN        = 0x04,
        BOTTOM_UP       = 0x05
    };
    static const TransitionMode TransitionMode_Last = BOTTOM_UP;

public:
    FlipdotDisplay(FlipdotDriver* drv_);
    ~FlipdotDisplay();

    void setDisplayMode(DisplayMode mode);
    void setTransitionMode(TransitionMode mode);
    void setPixelsPerSecond(uint16_t pixelsPerSecond);

    void clear();
    void fill();

    void display(const BitArray& new_state);
    void display_string(std::string s, PixelString::TextAlignment alignment = PixelString::LEFT);
    void flip_single_pixel(int x, int y, bool show);

    size_t copy_state(uint8_t* buffer, size_t len);
    bool get_pixel(int x, int y);

    inline int get_width() { return w; };
    inline int get_height() { return h; };
    inline int get_number_of_pixels() { return n; };

    inline bool is_state_known() { return !state_unknown; };
    bool is_valid_index(int x, int y);

    inline void setMutex(SemaphoreHandle_t m) { mutex = m; };

private:
    FlipdotDriver* drv;
    BitArray* state;
    bool state_unknown;

    PixelMap* cmap;
    size_t n, h, w;

    BitArray* transition_set;
    BitArray* transition_reset;

    DisplayMode displayMode;
    TransitionMode trxMode;

    uint16_t pixelsPerSecond;
    int flipTime;
    int waitTimeTask;
    int waitTimeUs;

    PixelCoord* randomTransitionMap;

    SemaphoreHandle_t mutex;

    void displayCurrentState();
    void reshuffleRandomTrxMap();
    inline void wait();
};


#endif //FLIPDOTDISPLAY_H
