#include "flipdotdisplay.h"

#include <vector>
#include "font/octafont-regular.h"
#include "font/octafont-bold.h"

#define TAG "flip-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
//#define ASCII_PIXELS
#include <esp_log.h>

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

void FlipdotDisplay::clear() {
    state->reset();
    display_current_state();
}

void FlipdotDisplay::fill() {
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

#ifdef ASCII_PIXELS
    PixelMap pixmap (drv->get_width(), drv->get_height(), true);
    for (size_t y = 0; y < drv->get_height(); y++) {
        string s = "";
        for (size_t x = 0; x < drv->get_width(); x++) {
            s += state->get(pixmap.index(x, y)) ? "x" : " ";
        }
        printf("|%s|\n", s.c_str());
    }
#endif

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

void FlipdotDisplay::flip_single_pixel(int x, int y, bool show, DisplayMode display_mode) {
    if (!state_unknown && display_mode == INCREMENTAL && state->get(cmap->index(x, y)) == show) {
        return;
    }

    // x and y bounds are checked in driver
    drv->flip(x, y, show);
    state->set(cmap->index(x, y), show);
}

size_t FlipdotDisplay::copy_state(uint8_t* buffer, size_t len) {
    return state->copy_to(buffer, len);
}

bool FlipdotDisplay::get_pixel(int x, int y) {
    return state->get(cmap->index(x, y));
}

void FlipdotDisplay::display_current_state() {
    for (int x = 0; x < drv->get_width(); x++) {
        for (int y = 0; y < drv->get_height(); y++) {
            drv->flip(x, y, state->get(x * drv->get_height() + y));
        }
    }

    state_unknown = false;
}

bool FlipdotDisplay::is_valid_index(int x, int y) {
    return 0 <= x && x < drv->get_width() &&
           0 <= y && y < drv->get_height();
}