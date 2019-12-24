#include "flipdotdisplay.h"

FlipdotDisplay::FlipdotDisplay(FlipdotDriver *drv_) {
    drv = drv_;
    state = new BitArray(drv->get_number_of_pixels());
    transition = new int8_t[drv->get_number_of_pixels()];
    clear();
}

FlipdotDisplay::~FlipdotDisplay() {
    delete state;
}

void FlipdotDisplay::init_by_test() {
    clear();
    light();
    clear();
    light();
    clear();
}

void FlipdotDisplay::clear() {
    state->setAll(false);
    display_current_state();
}

void FlipdotDisplay::light() {
    state->setAll(true);
    display_current_state();
}

void FlipdotDisplay::display_overriding(const BitArray &new_state) {
    state->copy_from(new_state);
    display_current_state();
}

void FlipdotDisplay::display_incrementally(const BitArray &new_state) {
    state->transition_vector_to(new_state, transition);
    state->copy_from(new_state);

    for (int x = 0; x < drv->get_width(); x++) {
        for (int y = 0; y < drv->get_height(); y++) {
            int t = transition[x * drv->get_height() + y];

            if (t) {
                drv->flip(x, y, t > 0 ? true : false);
            }
        }
    }
}

void FlipdotDisplay::display_string(std::string s) {
    BitArray new_state (drv->get_number_of_pixels());

    int x = 0;
    for (int i = 0; i < s.size(); i++) {
        const uint8_t* cptr = &font_data[s[i] * 5];

        for (int j = 0; j < 5; j++) {
            new_state.set8((x++)*16, *(cptr + j));
        }

        if (i < s.size() - 1) {
            x++;
        }
    }

    display_overriding(new_state);
}

void FlipdotDisplay::display_current_state() {
    for (int x = 0; x < drv->get_width(); x++) {
        for (int y = 0; y < drv->get_height(); y++) {
            drv->flip(x, y, state->get(x * drv->get_height() + y));
        }
    }
}

