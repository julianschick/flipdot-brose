#ifndef FLIPDOTDISPLAY_H
#define FLIPDOTDISPLAY_H

#include <string>

#include "util/bitarray.h"
#include "util/pixelmap.h"
#include "util/pixelstring.h"
#include "lowlevel/flipdotdriver.h"

class FlipdotDisplay {

public:
    enum DisplayMode {
        INCREMENTAL, OVERRIDE
    };

public:
    FlipdotDisplay(FlipdotDriver* drv_);
    ~FlipdotDisplay();

    void init_by_test();

    void clear();
    void light();

    void display(const BitArray& new_state, DisplayMode mode = OVERRIDE);
    void display_string(std::string s, PixelString::TextAlignment alignment = PixelString::LEFT, DisplayMode display_mode = OVERRIDE);

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
