#ifndef FLIPDOTDISPLAY_H
#define FLIPDOTDISPLAY_H

#include "util/bitarray.h"
#include "util/pixelmap.h"
#include "lowlevel/flipdotdriver.h"
#include <string>

enum TextAlignment {
    LEFT, RIGHT, CENTERED
};

enum DisplayMode {
    INCREMENTAL, OVERRIDE
};

class FlipdotDisplay {

public:
    FlipdotDisplay(FlipdotDriver* drv_);
    ~FlipdotDisplay();

    void init_by_test();

    void clear();
    void light();

    void display_overriding(const BitArray& new_state);
    void display_incrementally(const BitArray& new_state);

    void display_string(std::string s, TextAlignment alignment = LEFT, DisplayMode display_mode = OVERRIDE);

    inline int get_width() { return drv->get_width(); };
    inline int get_height() { return drv->get_height(); };
    inline int get_number_of_pixels() { return drv->get_number_of_pixels(); };

private:
    FlipdotDriver* drv;
    BitArray* state;
    bool state_unknown;
    PixelMap* cmap;

    BitArray* transition_set;
    BitArray* transition_reset;

    void display_current_state();

};


#endif //FLIPDOTDISPLAY_H
