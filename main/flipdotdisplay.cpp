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

void FlipdotDisplay::display_string(std::string s, TextAlignment alignment, DisplayMode display_mode) {
    BitArray new_state (drv->get_number_of_pixels());

    PixelCoord coords = {0, 0};
    PixelFont* font = new OctafontBold();

    std::vector<std::string> lines = split(s, '\n');

    for (int i = 0; i < lines.size() && i < 2; i++) {
        ESP_LOGI("display", "line = %s", lines[i].c_str());
        std::string& line = lines[i];
    }

    for (int i = 0; i < s.size(); i++) {
        char c = s[i];

        if (c == 32) {
            coords.x += 2;
            if (coords.x >= drv->get_width()) {
                coords.x = 0;
                coords.y += 8;
            }
        } else if (font->has_char(c)) {

            int width = font->get_width(c);

            ESP_LOGI("display", "char %d at %d, width = %d", c, coords.x, width);

            if (coords.x + width + 1 > drv->get_width()) {
                coords.x = 0;
                coords.y += 8;
            }

            if (coords.x != 0) {
                coords.x++;
            }

            for (size_t j = 0; j < width; j++) {
                new_state.set8(cmap->index(coords), font->get_octet(c, j));
                coords.x++;
            }
        }
    }

    display_overriding(new_state);
    delete font;
}

void FlipdotDisplay::display_current_state() {
    for (int x = 0; x < drv->get_width(); x++) {
        for (int y = 0; y < drv->get_height(); y++) {
            drv->flip(x, y, state->get(x * drv->get_height() + y));
        }
    }

    state_unknown = false;
}

