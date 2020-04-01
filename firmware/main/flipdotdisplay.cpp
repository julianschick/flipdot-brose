#include "flipdotdisplay.h"

#include <vector>
#include <algorithm>
#include "font/octafont-regular.h"
#include "font/octafont-bold.h"

#define TAG "flip-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>

// Show ASCII image of display
//#define ASCII_PIXELS

FlipdotDisplay::FlipdotDisplay(FlipdotDriver *drv_) {
    drv = drv_;
    state = new BitArray(drv->get_number_of_pixels());
    state_unknown = true;
    cmap = new PixelMap(drv->get_width(), drv->get_height(), true);

    transition_set = new BitArray(drv->get_number_of_pixels());
    transition_reset = new BitArray(drv->get_number_of_pixels());

    displayMode = INCREMENTAL;
    trxMode = LEFT_TO_RIGHT;
    overlayMode = NONE;

    buildTransitionMaps();
}

FlipdotDisplay::~FlipdotDisplay() {
    delete state;
    delete transition_set;
    delete transition_reset;
}

void FlipdotDisplay::setDisplayMode(DisplayMode mode) {
    ESP_LOGD(TAG, "Setting display mode %d", mode);
    displayMode = mode;
}

void FlipdotDisplay::setTransitionMode(TransitionMode mode) {
    ESP_LOGD(TAG, "Setting transition mode %d", mode);
    trxMode = mode;
}

void FlipdotDisplay::setOverlayMode(OverlayMode mode) {
    ESP_LOGD(TAG, "Setting overlay mode %d", mode);
    overlayMode = mode;
}

void FlipdotDisplay::clear() {
    if (displayMode == OVERRIDE) {
        state->reset();
        display_current_state();
    } else {
        BitArray blank (*state);
        blank.reset();
        display(blank);
    }
}

void FlipdotDisplay::fill() {
    if (displayMode == OVERRIDE) {
        state->set();
        display_current_state();
    } else {
        BitArray filled (*state);
        filled.set();
        display(filled);
    }
}

void FlipdotDisplay::display(const BitArray &new_state) {
    if (state_unknown || displayMode == OVERRIDE) {
        state->copy_from(new_state);
        display_current_state();
    } else {
        state->transition_vector_to(new_state, *transition_set, *transition_reset);
        state->copy_from(new_state);

        PixelCoord* trxMap = transitionMaps[trxMode];

        for (size_t i = 0; i < drv->get_number_of_pixels(); i++) {
            size_t idx = cmap->index(trxMap[i]);

            if ((*transition_set)[idx]) {
                drv->flip(trxMap[i], true);
            } else if ((*transition_reset)[idx]) {
                drv->flip(trxMap[i], false);
            }
        }

        if (trxMode == RANDOM) {
            reshuffleRandomTransitionMap();
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

void FlipdotDisplay::display_string(std::string s, PixelString::TextAlignment alignment) {
    BitArray new_state (drv->get_number_of_pixels());

    PixelString pixel_string(s);
    PixelMap pixmap (drv->get_width(), drv->get_height(), true);

    OctafontRegular font_regular;
    OctafontBold font_bold;

    pixel_string.print(new_state, pixmap, dynamic_cast<PixelFont&>(font_regular),dynamic_cast<PixelFont&>(font_bold), alignment);
    display(new_state);
}

void FlipdotDisplay::flip_single_pixel(int x, int y, bool show) {
    if (!state_unknown && displayMode == INCREMENTAL && state->get(cmap->index(x, y)) == show) {
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

    PixelCoord* trxMap = transitionMaps[trxMode];

    for (size_t i = 0; i < drv->get_number_of_pixels(); i++) {
        drv->flip(trxMap[i],state->get(cmap->index(trxMap[i])));
    }

    if (trxMode == RANDOM) {
        reshuffleRandomTransitionMap();
    }

    state_unknown = false;
}

bool FlipdotDisplay::is_valid_index(int x, int y) {
    return 0 <= x && x < drv->get_width() &&
           0 <= y && y < drv->get_height();
}

void FlipdotDisplay::buildTransitionMaps() {

    transitionMaps = new PixelCoord*[BOTTOM_UP + 1];
    int n = get_number_of_pixels();
    int h = get_height();
    int w = get_width();

    transitionMaps[0] = nullptr;

    transitionMaps[LEFT_TO_RIGHT] = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        transitionMaps[LEFT_TO_RIGHT][i].x = i / h;
        transitionMaps[LEFT_TO_RIGHT][i].y = i % h;
    }

    transitionMaps[RIGHT_TO_LEFT] = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        transitionMaps[RIGHT_TO_LEFT][i].x = w - 1 - (i / h);
        transitionMaps[RIGHT_TO_LEFT][i].y = i % h;
    }

    transitionMaps[RANDOM] = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        transitionMaps[RANDOM][i].x = i / h;
        transitionMaps[RANDOM][i].y = i % h;
    }
    reshuffleRandomTransitionMap();

    transitionMaps[TOP_DOWN] = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        transitionMaps[TOP_DOWN][i].x = i % w;
        transitionMaps[TOP_DOWN][i].y = i / w;
    }

    transitionMaps[BOTTOM_UP] = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        transitionMaps[BOTTOM_UP][i].x = i % w;
        transitionMaps[BOTTOM_UP][i].y = h - 1 - (i / w);
    }

}

void FlipdotDisplay::reshuffleRandomTransitionMap() {
    /*std::vector<PixelCoord> coords;
    for (size_t x = 0; x < get_width(); x++) {
        for (size_t y = 0; y < get_height(); y++) {
            PixelCoord c = {x, y};
            coords.push_back(c);
        }
    }
    std::random_shuffle(coords.begin(), coords.end());*/

    random_shuffle(transitionMaps[RANDOM], transitionMaps[RANDOM] + get_number_of_pixels());

    /*for (int i = 0; i < get_number_of_pixels(); i++) {
        transitionMaps[RANDOM][i] = coords[i];
    }*/
}
