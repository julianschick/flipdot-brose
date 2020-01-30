#include "flipdotdisplay.h"

#include <vector>
#include <esp_log.h>

#include "font/octafont-regular.h"
#include "font/octafont-bold.h"
#include "util/util.h"

FlipdotDisplay::FlipdotDisplay(FlipdotDriver *drv_) {
    drv = drv_;
    state = new BitArray(drv->get_number_of_pixels());
    state_unknown = true;
    cmap = new PixelMap(drv->get_width(), drv->get_height(), true);

    transition_set = new BitArray(drv->get_number_of_pixels());
    transition_reset = new BitArray(drv->get_number_of_pixels());
}

FlipdotDisplay::~FlipdotDisplay() {
    delete state;
    delete transition_set;
    delete transition_reset;
}

void FlipdotDisplay::init_by_test() {
    clear();
    light();
    clear();
    light();
    clear();
}

void FlipdotDisplay::clear() {
    state->reset();
    display_current_state();
}

void FlipdotDisplay::light() {
    state->set();
    display_current_state();
}

void FlipdotDisplay::display(const BitArray &new_state, DisplayMode mode) {
    if (state_unknown || mode == OVERRIDE) {
        state->copy_from(new_state);
        display_current_state();
    } else {
        state->transition_vector_to(new_state, *transition_set, *transition_reset);
        state->copy_from(new_state);

        for (size_t x = 0; x < drv->get_width(); x++) {
            for (size_t y = 0; y < drv->get_height(); y++) {
                size_t idx =  x * drv->get_height() + y;

                if ((*transition_set)[idx]) {
                    drv->flip(x, y, true);
                } else if ((*transition_reset)[idx]) {
                    drv->flip(x, y, false);
                }
            }
        }
    }

    PixelMap pixmap (drv->get_width(), drv->get_height(), true);
    for (size_t y = 0; y < drv->get_height(); y++) {
        string s = "";
        for (size_t x = 0; x < drv->get_width(); x++) {
            s += state->get(pixmap.index(x, y)) ? "x" : " ";
        }
        printf("|%s|\n", s.c_str());
    }

    printf("display done\n");

}

void FlipdotDisplay::display_string(std::string s, PixelString::TextAlignment alignment, DisplayMode display_mode) {
    BitArray new_state (drv->get_number_of_pixels());

    PixelString pixel_string(s);
    PixelMap pixmap (drv->get_width(), drv->get_height(), true);

    OctafontRegular font_regular;
    OctafontBold font_bold;

    pixel_string.print(new_state, pixmap, dynamic_cast<PixelFont&>(font_regular),dynamic_cast<PixelFont&>(font_bold), alignment);
    display(new_state, display_mode);
}

void FlipdotDisplay::display_current_state() {
    for (int x = 0; x < drv->get_width(); x++) {
        for (int y = 0; y < drv->get_height(); y++) {
            drv->flip(x, y, state->get(x * drv->get_height() + y));
        }
    }

    state_unknown = false;
}

