#include "ws2812controller.h"

#include <nvs.h>
#include <nvs_flash.h>
#include <freertos/task.h>

#define TAG "ws2812-controller"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

#define PREPARE_MSG_VAL(msg, cmd_enum, ptr_type, value) msg.cmd = cmd_enum; ptr_type* tmp = new ptr_type; *tmp = value; msg.data = tmp;
#define PREPARE_MSG_PTR(msg, cmd_enum, ptr_type, value) msg.cmd = cmd_enum; ptr_type* tmp = new ptr_type; tmp = value; msg.data = tmp;

WS2812Controller::WS2812Controller(WS2812Driver *drv) : drv(drv) {
    transitionMode = LINEAR_SLOW;

    n = drv->get_led_count();
    state = drv->get_state();

    startState = new color_t[n];
    endState = new color_t[n];

    queue = xQueueCreate(10, sizeof(LedCommandMsg));
    mutex = xSemaphoreCreateMutex();
}

WS2812Controller::~WS2812Controller() {
    delete drv;
    vQueueDelete(queue);
    vSemaphoreDelete(mutex);
}

bool WS2812Controller::setTransitionMode(WS2812Controller::TransitionMode mode) {
    LedCommandMsg msg;
    PREPARE_MSG_VAL(msg, SET_LED_TRX_MODE, TransitionMode, mode)
    return xQueueSend(queue, &msg, portMAX_DELAY) == pdTRUE;
}

void WS2812Controller::setTransitionMode_(TransitionMode* mode) {
    transitionMode = *mode;
    delete mode;
}

bool WS2812Controller::setAllLedsToSameColor(color_t color) {
    LedCommandMsg msg;
    PREPARE_MSG_VAL(msg, SET_ALL_LEDS, color_t, color)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

void WS2812Controller::setAllLedsToSameColor_(color_t* c) {
    for (int i = 0; i < n; i++) {
        endState[i] = *c;
    }
    delete c;
    startTransition();
}

bool WS2812Controller::setLeds(std::vector<LedChangeCommand> *changes) {
    LedCommandMsg msg;
    PREPARE_MSG_PTR(msg, SET_LEDS, std::vector<LedChangeCommand>, changes)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

void WS2812Controller::setLeds_(std::vector<LedChangeCommand> *changes) {
    for (size_t i = 0; i < changes->size(); i++) {
        ESP_LOGV(TAG, "led %d, to (%d, %d, %d)", changes->at(i).addr, changes->at(i).color.brg.red, changes->at(i).color.brg.green, changes->at(i).color.brg.blue);
        endState[changes->at(i).addr] = changes->at(i).color;
    }
    startTransition();
    delete changes;
}

color_t* WS2812Controller::getAll() {
    color_t* result = new color_t[n];
    xSemaphoreTake(mutex, portMAX_DELAY);
    copy(result, state);
    xSemaphoreGive(mutex);
    return result;
}

void WS2812Controller::cycle() {

    if (!inTransition) {
        LedCommandMsg msg;
        if (xQueueReceive(queue, &msg, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            executeCommand(msg);
        } else {
            // save state not more frequently than every 10 seconds
            if (newState && (xTaskGetTickCount() - lastStateSave)*portTICK_PERIOD_MS > 10*1000) {
                saveState();
            }
        }
        return;
    }

    transitionPosition++;
    float pos = (float) transitionPosition / 255.0;

    int64_t now = esp_timer_get_time();
    int elapsed = (now - lastCycle) / 1000;
    lastCycle = now;
    int targetDelay = getDelayByMode(transitionMode);
    int actualDelay = targetDelay - elapsed;

    ESP_LOGV(TAG, "elapsed %d", elapsed);
    ESP_LOGV(TAG, "targetDelay %d", targetDelay);
    ESP_LOGV(TAG, "actualDelay %d", actualDelay);
    ESP_LOGV(TAG, "pos %d", transitionPosition);

    if (actualDelay > 0) {
        vTaskDelay(actualDelay / portTICK_PERIOD_MS);
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    for (size_t i = 0; i < n; i++) {
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
    xSemaphoreGive(mutex);

    ESP_LOGV(TAG, "LED[%d] = rgb(%#02x, %#02x, %#02x, %#02x)", 0, state[0].brg.red, state[0].brg.green, state[0].brg.blue, state[0].brg.padding);

    drv->update();

    if (transitionPosition == 255) {
        inTransition = false;
        if (!loadingFromNvs) {
            newState = true;
        } else {
            loadingFromNvs = false;
        }
    }
}

bool WS2812Controller::isBusy() {
    return inTransition;
}

void WS2812Controller::startTransition() {
    if (transitionMode == IMMEDIATE) {
        copy(state, endState);
        inTransition = false;
        if (!loadingFromNvs) {
            newState = true;
        } else {
            loadingFromNvs = false;
        }
        drv->update();
        return;
    }

    copy(startState, state);
    inTransition = true;

    transitionPosition = 0.0;
    lastCycle = esp_timer_get_time();
}

void WS2812Controller::copy(color_t *dest, color_t *src) {
    xthal_memcpy(dest, src, sizeof(color_t)*n);
}

void WS2812Controller::executeCommand(WS2812Controller::LedCommandMsg &msg) {

    if (msg.cmd == SET_ALL_LEDS) {
        setAllLedsToSameColor_((color_t *) msg.data);
    } else if (msg.cmd == SET_LED_TRX_MODE) {
        setTransitionMode_((TransitionMode*) msg.data);
    } else if (msg.cmd == SET_LEDS) {
        setLeds_((std::vector<LedChangeCommand>*) msg.data);
    }

}

void WS2812Controller::saveState() {
    ESP_LOGI(TAG, "Saving state to flash");

    /*nvs_handle_t nvs;
    nvs_open("led", NVS_READWRITE, &nvs);
    nvs_set_blob(nvs, "state", state, sizeof(color_t)*size);
    nvs_commit(nvs);
    nvs_close(nvs);*/

    newState = false;
    lastStateSave = xTaskGetTickCount();
}

void WS2812Controller::readState() {
    ESP_LOGI(TAG, "Reading state from flash");

    size_t size = sizeof(color_t)*n;
    nvs_handle_t nvs;
    nvs_open("led", NVS_READONLY, &nvs);
    nvs_get_blob(nvs, "state", endState, &size);
    nvs_close(nvs);

    lastStateSave = xTaskGetTickCount();
    loadingFromNvs = true;
    startTransition();
}
