#include "flipdotbuffer.h"

#define PREPARE_MSG_VAL(msg, cmd_enum, ptr_type, value) msg.cmd = cmd_enum; ptr_type* tmp = new ptr_type; *tmp = value; msg.data = tmp;

FlipdotBuffer::FlipdotBuffer(FlipdotDisplay *ctrl, size_t queueLength) : ctrl(ctrl) {
    queue = xQueueCreate(queueLength, sizeof(FlipdotCommandMsg));
    mutex = xSemaphoreCreateMutex();
    ctrl->setMutex(mutex);
}

FlipdotBuffer::~FlipdotBuffer() {
    ctrl->setMutex(nullptr);
    vQueueDelete(queue);
    vSemaphoreDelete(mutex);
}

bool FlipdotBuffer::executeNext(int timeout) {
    if (xQueueReceive(queue, &msg, timeout / portTICK_PERIOD_MS) == pdTRUE) {
        executeCommand(msg);
        return true;
    } else {
        return false;
    }
}

bool FlipdotBuffer::showBitset(BitArray* bitset) {
    FlipdotCommandMsg msg = { SHOW_BITSET, bitset};
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::clear() {
    FlipdotCommandMsg msg = { CLEAR, nullptr};
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::fill() {
    FlipdotCommandMsg msg = { FILL, nullptr};
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::setPixel(int x, int y, bool visible) {
    SetPixelCommand* cmd = new SetPixelCommand(x, y, visible);
    FlipdotCommandMsg msg = { SET_PIXELS, cmd};
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::scroll(std::vector<uint16_t>* dataAndMask) {
    FlipdotCommandMsg msg = { SCROLL, dataAndMask };
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::setDisplayMode(FlipdotDisplay::DisplayMode displayMode) {
    FlipdotCommandMsg msg;
    PREPARE_MSG_VAL(msg, SET_DISPLAY_MODE, FlipdotDisplay::DisplayMode, displayMode)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::setFlipSpeed(uint16_t pixelsPerSecond) {
    FlipdotCommandMsg msg;
    PREPARE_MSG_VAL(msg, SET_FLIP_SPEED, uint16_t, pixelsPerSecond)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::setTransitionMode(FlipdotDisplay::TransitionMode trxMode) {
    FlipdotCommandMsg msg;
    PREPARE_MSG_VAL(msg, SET_TRX_MODE, FlipdotDisplay::TransitionMode, trxMode)
    return xQueueSend(queue, &msg, 0) == pdTRUE;
}

bool FlipdotBuffer::isStateKnown() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    bool stateKnown = ctrl->is_state_known();
    xSemaphoreGive(mutex);
    return stateKnown;
}

bool FlipdotBuffer::getPixel(int x, int y) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    bool visible = ctrl->get_pixel(x, y);
    xSemaphoreGive(mutex);
    return visible;
}

size_t FlipdotBuffer::getBitset(uint8_t *buffer, size_t len) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    size_t result = ctrl->copy_state(buffer, len);
    xSemaphoreGive(mutex);
    return result;
}

FlipdotDisplay::DisplayMode FlipdotBuffer::getDisplayMode() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    FlipdotDisplay::DisplayMode result = ctrl->getDisplayMode();
    xSemaphoreGive(mutex);
    return result;
}

uint16_t FlipdotBuffer::getFlipSpeed() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    uint16_t result = ctrl->getPixelsPerSecond();
    xSemaphoreGive(mutex);
    return result;
}

FlipdotDisplay::TransitionMode FlipdotBuffer::getTransitionMode() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    FlipdotDisplay::TransitionMode result = ctrl->getTransitionMode();
    xSemaphoreGive(mutex);
    return result;
}

void FlipdotBuffer::executeCommand(FlipdotBuffer::FlipdotCommandMsg &msg) {

    switch (msg.cmd) {

        case SHOW_BITSET: {
            BitArray *b = (BitArray *) msg.data;
            ctrl->display(*b);
            delete b;
        }   break;

        case CLEAR:
            ctrl->clear();
            break;

        case FILL:
            ctrl->fill();
            break;

        case SET_PIXELS: {
            SetPixelCommand *cmd = (SetPixelCommand *) msg.data;
            ctrl->flip_single_pixel(cmd->x, cmd->y, cmd->visible);
            delete cmd;
        }   break;

        case SCROLL: {
            std::vector<uint16_t>* dataAndMask = (std::vector<uint16_t>*) msg.data;
            ctrl->scroll(dataAndMask);
            delete dataAndMask;
        }   break;

        case SET_DISPLAY_MODE: {
            FlipdotDisplay::DisplayMode *mode = (FlipdotDisplay::DisplayMode *) msg.data;
            ctrl->setDisplayMode(*mode);
            delete mode;
        }   break;

        case SET_FLIP_SPEED: {
            uint16_t *pixelsPerSecond = (uint16_t *) msg.data;
            ctrl->setPixelsPerSecond(*pixelsPerSecond);
            delete pixelsPerSecond;
        }   break;

        case SET_TRX_MODE: {
            FlipdotDisplay::TransitionMode *mode = (FlipdotDisplay::TransitionMode*) msg.data;
            ctrl->setTransitionMode(*mode);
            delete mode;
        }   break;

    }
}