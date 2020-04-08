#include "ws2812controller.h"

#include <nvs.h>
#include <nvs_flash.h>
#include <freertos/task.h>

#define TAG "ws2812-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

WS2812Controller::WS2812Controller(WS2812Driver *drv) : drv(drv) {
    transitionMode = LINEAR_SLOW;

    n = drv->get_led_count();
    state = drv->get_state();

    startState = new color_t[n];
    endState = new color_t[n];

    ESP_LOGI(TAG, "Initialized on %d LEDs", n);
}

WS2812Controller::~WS2812Controller() {
    delete drv;
}

void WS2812Controller::setTransitionMode(TransitionMode mode) {
    ESP_LOGD(TAG, "Set transition mode from %d to %d", transitionMode, mode);
    transitionMode = mode;
}

void WS2812Controller::setAllLedsToSameColor(color_t c) {
    ESP_LOGD(TAG, "Set all LEDs to rgb(%#02x, %#02x, %#02x)", c.brg.red, c.brg.green, c.brg.blue);
    for (int i = 0; i < n; i++) {
        endState[i] = c;
    }
    transition();
}

void WS2812Controller::setLeds(std::vector<LedChangeCommand>* changes) {
    ESP_LOGD(TAG, "%d LED commands", changes->size());
    for (size_t i = 0; i < changes->size(); i++) {
        ESP_LOGV(TAG, "LED %d to rgb(%#02x, %#02x, %#02x)", changes->at(i).addr, changes->at(i).color.brg.red, changes->at(i).color.brg.green, changes->at(i).color.brg.blue);
        endState[changes->at(i).addr] = changes->at(i).color;
    }
    transition();
}

color_t* WS2812Controller::getAll() {
    return state;
}

void WS2812Controller::transition() {

    if (transitionMode == IMMEDIATE) {
        ESP_LOGD(TAG, "Immediate transition");
        lock();
        copy(state, endState);
        unlock();

        if (!loadingFromNvs) {
            newState = true;
        } else {
            loadingFromNvs = false;
        }
        drv->update();
        return;
    }

    ESP_LOGD(TAG, "Long transition (mode = %d).", transitionMode);
    copy(startState, state);
    lastCycle = esp_timer_get_time();
    float pos = 0;

    for (uint8_t transitionPosition = 0; transitionPosition > 0 || pos == 0; transitionPosition++) {
        pos = (float) transitionPosition / 255.0;

        int64_t now = esp_timer_get_time();
        int elapsed = (now - lastCycle) / 1000;
        lastCycle = now;
        int targetDelay = getDelayByMode(transitionMode);
        int actualDelay = targetDelay - elapsed;

        ESP_LOGV(TAG, "elapsed = %d", elapsed);
        ESP_LOGV(TAG, "targetDelay = %d", targetDelay);
        ESP_LOGV(TAG, "actualDelay = %d", actualDelay);
        ESP_LOGV(TAG, "pos = %d / 255", transitionPosition);

        if (actualDelay > 0) {
            vTaskDelay(actualDelay / portTICK_PERIOD_MS);
        }

        lock();
        for (size_t i = 0; i < n; i++) {
            ESP_LOGV(TAG, "LED[%d] = rgb(%#02x, %#02x, %#02x)", i, state[0].brg.red, state[0].brg.green, state[0].brg.blue);

            switch (transitionMode) {
                case LINEAR_QUICK:
                case LINEAR_MEDIUM:
                case LINEAR_SLOW:

                    state[i].brg.red = (float) startState[i].brg.red * (1 - pos) +
                                       (float) endState[i].brg.red * pos;
                    state[i].brg.green = (float) startState[i].brg.green * (1 - pos) +
                                         (float) endState[i].brg.green * pos;
                    state[i].brg.blue = (float) startState[i].brg.blue * (1 - pos) +
                                        (float) endState[i].brg.blue * pos;

                    break;
                case SLIDE_QUICK:
                case SLIDE_MEDIUM:
                case SLIDE_SLOW:
                    state[i].brg = i < pos * n ? endState[i].brg : startState[i].brg;
                    break;


                default:
                    break;
            }
        }
        unlock();

        drv->update();
    }

    ESP_LOGD(TAG, "Long transition finished");
    if (!loadingFromNvs) {
        newState = true;
    } else {
        loadingFromNvs = false;
    }

}

void WS2812Controller::copy(color_t *dest, color_t *src) {
    xthal_memcpy(dest, src, sizeof(color_t)*n);
}


bool WS2812Controller::saveStateIfNecessary() {
    if (!newState) {
        return false;
    }

    saveState();
    return true;
}

void WS2812Controller::saveState() {
    ESP_LOGI(TAG, "Saving state to flash");

    /*nvs_handle_t nvs;
    nvs_open("led", NVS_READWRITE, &nvs);
    nvs_set_blob(nvs, "state", state, sizeof(color_t)*size);
    nvs_commit(nvs);
    nvs_close(nvs);*/

    newState = false;
}

void WS2812Controller::readState() {
    ESP_LOGI(TAG, "Reading state from flash");

    size_t size = sizeof(color_t)*n;
    nvs_handle_t nvs;
    nvs_open("led", NVS_READONLY, &nvs);
    nvs_get_blob(nvs, "state", endState, &size);
    nvs_close(nvs);

    loadingFromNvs = true;
    transition();
}
