#include "ws2812buffer.h"

#define PREPARE_MSG_VAL(msg, cmd_enum, ptr_type, value) msg.cmd = cmd_enum; ptr_type* tmp = new ptr_type; *tmp = value; msg.data = tmp;
#define PREPARE_MSG_PTR(msg, cmd_enum, ptr_type, value) msg.cmd = cmd_enum; ptr_type* tmp = new ptr_type; tmp = value; msg.data = tmp;

WS2812Buffer::WS2812Buffer(WS2812Controller *ctrl, size_t queueLength) : ctrl(ctrl) {
    queue = xQueueCreate(queueLength, sizeof(WS2812Controller::LedCommandMsg));
    mutex = xSemaphoreCreateMutex();
    ctrl->setMutex(mutex);
}

WS2812Buffer::~WS2812Buffer() {
    ctrl->setMutex(nullptr);
    vQueueDelete(queue);
    vSemaphoreDelete(mutex);
}

bool WS2812Buffer::executeNext(int timeout) {
    if (xQueueReceive(queue, &msg, timeout / portTICK_PERIOD_MS) == pdTRUE) {
        executeCommand(msg);
        return true;
    } else {
        return false;
    }
}

bool WS2812Buffer::setTransitionMode(WS2812Controller::TransitionMode mode) {
    WS2812Controller::LedCommandMsg msg;
    PREPARE_MSG_VAL(msg, WS2812Controller::SET_LED_TRX_MODE, WS2812Controller::TransitionMode, mode)
    return xQueueSend(queue, &msg, portMAX_DELAY) == pdTRUE;
}

bool WS2812Buffer::setAllLedsToSameColor(color_t color) {
    WS2812Controller::LedCommandMsg msg;
    PREPARE_MSG_VAL(msg, WS2812Controller::SET_ALL_LEDS, color_t, color)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool WS2812Buffer::setLeds(std::vector<WS2812Controller::LedChangeCommand> *changes) {
    WS2812Controller::LedCommandMsg msg;
    PREPARE_MSG_PTR(msg, WS2812Controller::SET_LEDS, std::vector<WS2812Controller::LedChangeCommand>, changes)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

color_t* WS2812Buffer::getAll() {
    color_t *result = new color_t[ctrl->getLedCount()];
    xSemaphoreTake(mutex, portMAX_DELAY);
    xthal_memcpy(result, ctrl->getAll(), ctrl->getLedCount());
    xSemaphoreGive(mutex);
    return result;
}

void WS2812Buffer::executeCommand(WS2812Controller::LedCommandMsg& msg) {
    if (msg.cmd == WS2812Controller::SET_ALL_LEDS) {
        color_t* c = (color_t*) msg.data;
        ctrl->setAllLedsToSameColor(*c);
        delete c;
    } else if (msg.cmd == WS2812Controller::SET_LED_TRX_MODE) {
        WS2812Controller::TransitionMode* trxMode = (WS2812Controller::TransitionMode*) msg.data;
        ctrl->setTransitionMode(*trxMode);
        delete trxMode;
    } else if (msg.cmd == WS2812Controller::SET_LEDS) {
        std::vector<WS2812Controller::LedChangeCommand>* changeCommands = (std::vector<WS2812Controller::LedChangeCommand>*) msg.data;
        ctrl->setLeds(changeCommands);
        delete changeCommands;
    }
}