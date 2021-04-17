#include "flipdotdisplay.h"

#include <vector>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "flip-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
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
    if (displayMode == OVERRIDE) {
        ESP_LOGD(TAG, "Clear display (OVERRIDE)");
        lock(); state->reset(); unlock();
        displayCurrentState();
    } else {
        ESP_LOGD(TAG, "Clear display (INCREMENTAL)");
        BitArray blank (*state);
        blank.reset();
        display(blank);
    }
}

void FlipdotDisplay::fill() {
    if (displayMode == OVERRIDE) {
        ESP_LOGD(TAG, "Fill display (OVERRIDE)");
        state->set();
        displayCurrentState();
    } else {
        ESP_LOGD(TAG, "Fill display (INCREMENTAL)");
        BitArray filled (*state);
        filled.set();
        display(filled);
    }
}

void FlipdotDisplay::display(const BitArray &new_state) {
    if (state_unknown || displayMode == OVERRIDE) {
        ESP_LOGD(TAG, "Change display (OVERRIDE)");
        lock(); state->copy_from(new_state); unlock();
        displayCurrentState();
    } else {
        ESP_LOGD(TAG, "Change display (INCREMENTAL)");
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

void FlipdotDisplay::flip_single_pixel(int x, int y, bool show) {
    if (!state_unknown && displayMode == INCREMENTAL && state->get(cmap->index(x, y)) == show) {
        return;
    }

    // x and y bounds are checked in driver
    drv->flip(x, y, show);
    lock(); state->set(cmap->index(x, y), show); unlock();
}

void FlipdotDisplay::scroll(std::vector<uint16_t>* dataAndMask) {

    BitArray new_state = BitArray(n);

    if (h % 8 == 0) {
        int bytes_per_col = h / 8;

        for (int x = 0; x < w; x++) {
            for(int y_byte = 0; y_byte < bytes_per_col && y_byte < dataAndMask->size(); y_byte++) {

                size_t index = cmap->index(x, y_byte * 8);
                uint8_t old_data = state->get8(index);
                uint8_t shift_mask = (dataAndMask->at(y_byte) & 0xFF00) >> 8;
                uint8_t shift_data = 0;

                if (x < w - 1) {
                    shift_data = state->get8(cmap->index(x+1, y_byte*8));
                } else {
                    shift_data = dataAndMask->at(y_byte) & 0x00FF;
                }

                uint8_t new_data = (old_data & ~shift_mask) | (shift_data & shift_mask);

                ESP_LOGD(TAG, "(%d, %d) old_data =   %02x", x, y_byte, old_data);
                ESP_LOGD(TAG, "(%d, %d) shift_mask = %02x", x, y_byte, shift_mask);
                ESP_LOGD(TAG, "(%d, %d) shift_data = %02x", x, y_byte, shift_data);
                ESP_LOGD(TAG, "(%d, %d) new_data =   %02x", x, y_byte, new_data);

                new_state.set8(index, new_data);
            }
        }

    } else {

        bool row_shifted[h];
        bool new_bit[h];

        for (int y = 0; y < h && y / 8 < dataAndMask->size(); y++) {
            row_shifted[y] = ((dataAndMask->at(y / 8) >> (8 + (y % 8))) & 0x0001) > 0;
            new_bit[y] = ((dataAndMask->at(y / 8) >> (y % 8)) & 0x0001) > 0;

            ESP_LOGD(TAG, "row_shifted[%d] = %d", y, row_shifted[y]);
            ESP_LOGD(TAG, "new_bit[%d] = %d", y, new_bit[y]);
        }

        for (int y = 0; y < h; y++) {
            if (row_shifted[y]) {
                for (int x = 0; x < w - 1; x++) {
                    new_state.set(cmap->index(x, y), state->get(cmap->index(x + 1, y)));
                }
                new_state.set(cmap->index(w - 1, y), new_bit[y]);
            } else {
                for (int x = 0; x < w; x++) {
                    new_state.set(cmap->index(x, y), state->get(cmap->index(x, y)));
                }
            }
        }
    }

    display(new_state);
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
    std::random_shuffle(randomTransitionMap, randomTransitionMap + n);
}

void FlipdotDisplay::wait() {
    vTaskDelay(waitTimeTask);
    ets_delay_us(waitTimeUs);
}
