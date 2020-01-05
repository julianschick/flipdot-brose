#include "flipdotdisplay.h"

FlipdotDisplay::FlipdotDisplay(FlipdotDriver *drv_) {
    drv = drv_;
    state = new BitArray(drv->get_number_of_pixels());
    state_unknown = true;
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

void FlipdotDisplay::display_overriding(const BitArray &new_state) {
    state->copy_from(new_state);
    display_current_state();
}

void FlipdotDisplay::display_incrementally(const BitArray &new_state) {
    if (state_unknown) {
        display_overriding(new_state);
        return;
    }

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

    state_unknown = false;
}

