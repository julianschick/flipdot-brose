#ifndef FLIPDOTDISPLAY_H
#define FLIPDOTDISPLAY_H

#include <string>

#include "util/bitarray.h"
#include "util/pixelmap.h"
#include "util/pixelstring.h"
#include "drivers/flipdotdriver.h"

class FlipdotDisplay {

public:
    enum DisplayMode {
        OVERRIDE        = 0x00,
        INCREMENTAL     = 0x01
    };

    enum TransitionMode {
        LEFT_TO_RIGHT   = 0x01,
        RIGHT_TO_LEFT   = 0x02,
        RANDOM          = 0x03,
        TOP_DOWN        = 0x04,
        BOTTOM_UP       = 0x05
    };

    enum OverlayMode {
        NONE            = 0x00,
        POSITIVE        = 0x01,
        NEGATIVE        = 0x02
    };

public:
    FlipdotDisplay(FlipdotDriver* drv_);
    ~FlipdotDisplay();

    void setDisplayMode(DisplayMode mode);
    void setTransitionMode(TransitionMode mode);
    void setOverlayMode(OverlayMode mode);

    void clear();
    void fill();

    void display(const BitArray& new_state);
    void display_string(std::string s, PixelString::TextAlignment alignment = PixelString::LEFT);
    void flip_single_pixel(int x, int y, bool show);

    size_t copy_state(uint8_t* buffer, size_t len);
    bool get_pixel(int x, int y);

    inline int get_width() { return drv->get_width(); };
    inline int get_height() { return drv->get_height(); };
    inline int get_number_of_pixels() { return drv->get_number_of_pixels(); };

    inline bool is_state_known() { return !state_unknown; };
    bool is_valid_index(int x, int y);

private:
    FlipdotDriver* drv;
    BitArray* state;
    bool state_unknown;
    PixelMap* cmap;

    BitArray* transition_set;
    BitArray* transition_reset;

    DisplayMode displayMode;
    TransitionMode trxMode;
    OverlayMode overlayMode;

    PixelCoord** transitionMaps;

    void display_current_state();
    void buildTransitionMaps();
    void reshuffleRandomTransitionMap();

};


#endif //FLIPDOTDISPLAY_H
