#include "flipdotdisplay.h"

#include <vector>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../font/octafont-regular.h"
#include "../font/octafont-bold.h"

#define TAG "flip-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include <esp_log.h>

// Show ASCII image of display
//#define ASCII_PIXELS

FlipdotDisplay::FlipdotDisplay(FlipdotDriver *drv_) {
    drv = drv_;

    n = drv->get_number_of_pixels();
    h = drv->get_height();
    w = drv->get_width();
    cmap = new PixelMap(w, h, true);

    state = new BitArray(n);
    state_unknown = true;
    transition_set = new BitArray(n);
    transition_reset = new BitArray(n);

    displayMode = INCREMENTAL;
    trxMode = LEFT_TO_RIGHT;

    mutex = nullptr;

    randomTransitionMap = new PixelCoord[n];
    for (int i = 0; i < n; i++) {
        randomTransitionMap[i].x = i / h;
        randomTransitionMap[i].y = i % h;
    }

    setPixelsPerSecond(5000);
    reshuffleRandomTrxMap();
}

FlipdotDisplay::~FlipdotDisplay() {
    delete state;
    delete transition_set;
    delete transition_reset;
}

void FlipdotDisplay::setDisplayMode(DisplayMode mode) {
    ESP_LOGD(TAG, "Setting display mode %d", mode);
    lock();
    displayMode = mode;
    unlock();
}

void FlipdotDisplay::setTransitionMode(TransitionMode mode) {
    ESP_LOGD(TAG, "Setting transition mode %d", mode);
    lock();
    trxMode = mode;
    unlock();
}

void FlipdotDisplay::setPixelsPerSecond(uint16_t pixelsPerSecond) {
    ESP_LOGD(TAG, "Setting pixels per second to %d", pixelsPerSecond);
    lock();
    this->pixelsPerSecond = pixelsPerSecond;
    unlock();

    flipdot_driver_timing_config_t config;

    // in usecs
    int waitTime = 0;

    if (pixelsPerSecond <= 1000) {
        flipTime = 1000;
        waitTime = 1000000 / pixelsPerSecond - 1000;
    } else {
        flipTime = 1000000 / pixelsPerSecond;
        waitTime = 0;
    }

    // safety check
    if (flipTime > 1000) {
        flipTime = 1000;
    }
    drv->set_timing(flipTime);

    waitTimeTask = waitTime / (portTICK_PERIOD_MS*1000);
    waitTimeUs = waitTime % (portTICK_PERIOD_MS*1000);
    ESP_LOGD(TAG, "waitTimeTask = %d, waitTimeUs = %d, flipTime = %d", waitTimeTask, waitTimeUs, flipTime);
}

void FlipdotDisplay::clear() {
    ESP_LOGD(TAG, "Clear display");

    if (displayMode == OVERRIDE) {
        lock(); state->reset(); unlock();
        displayCurrentState();
    } else {
        BitArray blank (*state);
        blank.reset();
        display(blank);
    }
}

void FlipdotDisplay::fill() {
    ESP_LOGD(TAG, "Fill display");

    if (displayMode == OVERRIDE) {
        state->set();
        displayCurrentState();
    } else {
        BitArray filled (*state);
        filled.set();
        display(filled);
    }
}

void FlipdotDisplay::display(const BitArray &new_state) {
    if (state_unknown || displayMode == OVERRIDE) {
        lock(); state->copy_from(new_state); unlock();
        displayCurrentState();
    } else {
        lock();
        state->transition_vector_to(new_state, *transition_set, *transition_reset);
        state->copy_from(new_state);
        unlock();
        PixelCoord c;

        for (size_t i = 0; i < drv->get_number_of_pixels(); i++) {
            switch (trxMode) {
                case LEFT_TO_RIGHT:         LEFT_TO_RIGHT_COORD(c); break;
                case RIGHT_TO_LEFT:         RIGHT_TO_LEFT_COORD(c); break;
                case RANDOM:                RANDOM_COORD(c); break;
                case TOP_DOWN:              TOP_DOWN_COORD(c); break;
                case BOTTOM_UP:             BOTTOM_UP_COORD(c); break;
            }
            size_t idx = cmap->index(c);

            if ((*transition_set)[idx]) {
                drv->flip(c, true);
                if (waitTimeTask > 0 || waitTimeUs > 0) {
                    wait();
                }
            } else if ((*transition_reset)[idx]) {
                drv->flip(c, false);
                if (waitTimeTask > 0 || waitTimeUs > 0) {
                    wait();
                }
            }
        }

        if (trxMode == RANDOM) {
            reshuffleRandomTrxMap();
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
    BitArray new_state (n);
    OctafontRegular font_regular;
    OctafontBold font_bold;

    PixelString pixel_string(s);
    pixel_string.print(new_state, *cmap, dynamic_cast<PixelFont&>(font_regular),dynamic_cast<PixelFont&>(font_bold), alignment);
    display(new_state);
}

void FlipdotDisplay::flip_single_pixel(int x, int y, bool show) {
    if (!state_unknown && displayMode == INCREMENTAL && state->get(cmap->index(x, y)) == show) {
        return;
    }

    // x and y bounds are checked in driver
    drv->flip(x, y, show);
    lock(); state->set(cmap->index(x, y), show); unlock();
}

size_t FlipdotDisplay::copy_state(uint8_t* buffer, size_t len) {
    return state->copy_to(buffer, len);
}

bool FlipdotDisplay::get_pixel(int x, int y) {
    return state->get(cmap->index(x, y));
}

void FlipdotDisplay::displayCurrentState() {
    PixelCoord c;

    for (size_t i = 0; i < drv->get_number_of_pixels(); i++) {
        switch (trxMode) {
            case LEFT_TO_RIGHT:     LEFT_TO_RIGHT_COORD(c); break;
            case RIGHT_TO_LEFT:     RIGHT_TO_LEFT_COORD(c); break;
            case RANDOM:            RANDOM_COORD(c); break;
            case TOP_DOWN:          TOP_DOWN_COORD(c); break;
            case BOTTOM_UP:         BOTTOM_UP_COORD(c); break;
        }
        drv->flip(c, state->get(cmap->index(c)));
        if (waitTimeTask > 0 || waitTimeUs > 0) {
            wait();
        }
    }

    if (trxMode == RANDOM) {
        reshuffleRandomTrxMap();
    }

    state_unknown = false;
}

bool FlipdotDisplay::is_valid_index(int x, int y) {
    return 0 <= x && x < drv->get_width() &&
           0 <= y && y < drv->get_height();
}

void FlipdotDisplay::reshuffleRandomTrxMap() {
    random_shuffle(randomTransitionMap, randomTransitionMap + n);
}

void FlipdotDisplay::wait() {
    vTaskDelay(waitTimeTask);
    ets_delay_us(waitTimeUs);
}
