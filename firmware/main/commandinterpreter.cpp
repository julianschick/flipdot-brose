#include "commandinterpreter.h"

#include <string>
#include <esp_heap_trace.h>
#include "util/bitarray.h"

#define TAG "cmx"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO
#include <esp_log.h>

CommandInterpreter::CommandInterpreter(size_t buffer_size, void (*responseHandler) (const uint8_t*, size_t), void (*connectionCloseHandler)()) :
    buf(*(new RingBuffer(buffer_size, &outOfBoundHandler))),
    responseHandler(responseHandler),
    connectionCloseHandler(connectionCloseHandler) {

    for (int i = 0; i < 6; i++) {
        leds[i].r = 0;leds[i].g = 0; leds[i].b = 0;
    }
    for (int i = 0; i < 16; i++) {
        pixels[i] = false;
    }
}

CommandInterpreter::~CommandInterpreter() {
    delete &buf;
}

void CommandInterpreter::outOfBoundHandler(size_t index, size_t maxIndex) {
    ESP_LOGE(TAG, "Buffer index out of bounds: %d. Max index is %d.", index, maxIndex);
}

bool CommandInterpreter::queue(uint8_t *data, size_t len) {
    return buf.append(data, len);
}

bool CommandInterpreter::process() {

    if (buf.getLength() <= cursor) {
        return false;
    }

    uint8_t b = buf[cursor];
    size_t len = 0;

    switch (state) {
        case NEUTRAL: // cursor == 0

            if (b == 0xE5) {

                uint32_t wm = uxTaskGetStackHighWaterMark(ledTask);
                printf("led-task watermark = %d\n", wm);

                wm = uxTaskGetStackHighWaterMark(tcpServerTask);
                printf("tcp-task watermark = %d\n", wm);

                wm = uxTaskGetStackHighWaterMark(flipdotTask);
                printf("flipdot-task watermark = %d\n", wm);

                size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
                uint32_t min_free_heap = esp_get_minimum_free_heap_size();
                printf("free = %d, min free = %d\n", free_heap, min_free_heap);

                buf.removeLeading(1);
                respond(ACK);

            } else

            if (b == WHOAMI) {
                uint8_t r[2] = {0x70, 0x07};
                respond(&r[0], sizeof(r));
                buf.removeLeading(1);

            // show bitset
            } else if (b == 0x90) {
                state = SHOW_BITSET_LEN;
                cursor++;

            // clear display
            } else if (b == 0x91) {
                bool success = flipdotBuffer->clear();
                respond(success ? ACK : BUFFER_OVERFLOW);
                buf.removeLeading(1);

            // fill display
            } else if (b == 0x92) {
                bool success = flipdotBuffer->fill();
                respond(success ? ACK : BUFFER_OVERFLOW);
                buf.removeLeading(1);

            // show text message
            } else if (b == 0x93) {
                state = SHOW_TEXT_LEN_ALIGN;
                cursor++;

            // set pixel
            } else if (b == 0x94) {
                state = SET_PIXEL_NEXT;
                buf.removeLeading(1);

            // set all leds
            } else if (b == 0xA0) {
                state = SET_ALL_LEDS;
                buf.removeLeading(1);

            // set led
            } else if (b == 0xA1) {
                state = SET_LED_NEXT;
                buf.removeLeading(1);

            // get bitset
            } else if (b == 0xB0) {
                buf.removeLeading(1);

                if (!flipdotBuffer->isStateKnown()) {
                    respond(&STATE_UNKNOWN, 1);
                } else {
                    uint8_t replyData[(flipdotBuffer->getNumberOfPixels() / 8) + 1];
                    size_t replyLen = flipdotBuffer->getBitset(&replyData[0], sizeof(replyData));
                    respond(&replyData[0], replyLen);
                }

            // get pixel
            } else if (b == 0xB1) {
                buf.removeLeading(1);
                state = GET_PIXEL_X;

            // get all leds
            } else if (b == 0xC0) {
                buf.removeLeading(1);

                color_t *data = ledBuffer->getAll();
                uint8_t replyData[ledBuffer->getLedCount() * 3];

                for (size_t i = 0; i < ledBuffer->getLedCount(); i++) {
                    replyData[i * 3 + 0] = data[i].brg.red;
                    replyData[i * 3 + 1] = data[i].brg.green;
                    replyData[i * 3 + 2] = data[i].brg.blue;
                }
                delete[] data;

                respond(&replyData[0], sizeof(replyData));

            // set pixel incremental mode
            } else if (b == 0xD0) {
                buf.removeLeading(1);
                state = SET_INC_MODE;

            // set pixel flip speed
            } else if (b == 0xD1) {
                buf.removeLeading(1);
                state = SET_FLIP_SPEED;

            // set led transition mode
            } else if (b == 0xD2) {
                buf.removeLeading(1);
                state = SET_LED_TRX_MODE;

            // set pixel transition mode
            } else if (b == 0xD3) {
                buf.removeLeading(1);
                state = SET_TRX_MODE;

            // get pixel incremental mode
            } else if (b == 0xE0) {
                buf.removeLeading(1);
                respond(flipdotBuffer->getDisplayMode());

            // get pixel flip speed
            } else if (b == 0xE1) {
                buf.removeLeading(1);
                uint16_t flipSpeed = flipdotBuffer->getFlipSpeed();
                uint8_t response[2];
                response[0] = flipSpeed & 0x00FF;
                response[1] = (flipSpeed >> 8) & 0xFF;
                respond(&response[0], 2);

            // get led transition mode
            } else if (b == 0xE2) {
                buf.removeLeading(1);
                respond(ledBuffer->getTransitionMode());

            // get pixel transition mode
            } else if (b == 0xE3) {
                buf.removeLeading(1);
                respond(flipdotBuffer->getTransitionMode());

            // invalid command
            } else {
                ESP_LOGE(TAG, "invalid command %#04x\n", b);
                buf.removeLeading(1);
                invalidCommand();
            }

            break;

        NEXT_STATE(SHOW_BITSET_LEN, SHOW_BITSET_BITS)

        case SHOW_BITSET_BITS:
            len = buf[1];

            if (cursor - 1 < len) {
                cursor++;
            } else {
                uint8_t receivedBytes[len];
                for (size_t i = 0; i < len; i++) {
                    receivedBytes[i] = buf[i+2];
                }
                BitArray* bitset = new BitArray (flipdotBuffer->getNumberOfPixels());
                bitset->copy_from(&receivedBytes[0], len);
                bool success = flipdotBuffer->showBitset(bitset);

                revertCursor(success ? ACK : BUFFER_OVERFLOW);
            }
            break;

        NEXT_STATE(SHOW_TEXT_LEN_ALIGN, SHOW_TEXT_CHARS)

        case SHOW_TEXT_CHARS:
            len = (buf[1] & 0xFC) >> 2;

            if (cursor - 1 < len) {
                cursor++;
            } else {
                PixelString::TextAlignment alignment;
                if (!toAlignment(buf[1], &alignment)) {
                    revertCursor(ADDR_INVALID);
                } else {
                    uint8_t receivedBytes[len];
                    for (size_t i = 0; i < len; i++) {
                        receivedBytes[i] = buf[i + 2];
                    }

                    std::string str = std::string((char *) &receivedBytes[0], len);
                    ESP_LOGI(TAG, "string  of len %d received = %s", len, str.c_str());
                    bool success = flipdotBuffer->showText(str, alignment);
                    revertCursor(success ? ACK : BUFFER_OVERFLOW);
                }
            }
            break;

        case SET_PIXEL_NEXT:
            if (b == STOP) {
                buf.removeLeading(1);
                state = NEUTRAL;
            } else if (b == 0x00 || b == 0x01) {
                state = SET_PIXEL_X;
                cursor++;
            } else {
                buf.removeLeading(1);
                respond(INVALID_COMMAND);
                state = NEUTRAL;
            }
            break;

        NEXT_STATE(SET_PIXEL_X, SET_PIXEL_Y);

        case SET_PIXEL_Y: {
            bool visible = buf[0];
            uint8_t x = buf[1];
            uint8_t y = buf[2];

            ESP_LOGD(TAG, "(%d, %d) = %d", x, y, visible);

            if (!flipdotBuffer->isPixelValid(x, y)) {
                revertCursor(ADDR_INVALID);
            } else {
                bool success = flipdotBuffer->setPixel(x, y, visible);
                revertCursor(success ? ACK : BUFFER_OVERFLOW);

            }
            state = SET_PIXEL_NEXT;
        }   break;

        case SET_ALL_LEDS:
            if (cursor < 2) {
                cursor++;
            } else {

                color_t c = {{
                     (uint8_t) buf[2],
                     (uint8_t) buf[0],
                     (uint8_t) buf[1],
                     0xFF
                 }};

                bool success = ledBuffer->setAllLedsToSameColor(c);

                revertCursor(success ? ACK : BUFFER_OVERFLOW);
                state = SET_ALL_LEDS_NEXT;
            }
            break;

        case SET_ALL_LEDS_NEXT:
            if (buf[0] == STOP) {
                state = NEUTRAL;
            } else if (buf[0] == CONTINUE) {
                state = SET_ALL_LEDS;
            } else {
                state = NEUTRAL;
                invalidCommand();
            }
            buf.removeLeading(1);
            break;

        case SET_LED_NEXT:

            if (b == STOP) {

                if (cursor > 0) {
                    size_t n = cursor / 4;
                    std::vector<WS2812Controller::LedChangeCommand>* cmds =
                            new std::vector<WS2812Controller::LedChangeCommand>(n);

                    uint8_t response[n];
                    for (size_t i = 0; i < n; i++) {
                        uint8_t addr = 0x7F & buf[i*4 + 0];

                        if (addr < ledBuffer->getLedCount()) {
                            color_t c = {{
                                 (uint8_t) buf[i*4 + 3],
                                 (uint8_t) buf[i*4 + 1],
                                 (uint8_t) buf[i*4 + 2],
                                 0x00
                             }};

                            WS2812Controller::LedChangeCommand cmd = {addr, c};
                            cmds->push_back(cmd);
                            response[i] = ACK;
                        } else {
                            response[i] = ADDR_INVALID;
                        }
                    }

                    bool success = ledBuffer->setLeds(cmds);
                    if (!success) {
                        for (int i = 0; i < n; i++) {
                            if (response[i] == ACK) {
                                response[i] = BUFFER_OVERFLOW;
                            }
                        }
                    }

                    respond(&response[0], n);
                    revertCursor();
                } else {
                    state = NEUTRAL;
                    buf.removeLeading(1);
                    respond(ACK);
                }

            } else if ((buf[0] & 0x80) != 0) {
                state = NEUTRAL;
                buf.removeLeading(1);
                invalidCommand();
            } else {
                state = SET_LED_RGB;
                cursor++;
            }
            break;

        case SET_LED_RGB:
            cursor++;
            if (cursor % 4 == 0) {
                state = SET_LED_NEXT;
            }
            break;

        case GET_PIXEL_NEXT:
            if (buf[0] == STOP) {
                state = NEUTRAL;
            } else if (buf[0] == CONTINUE) {
                state = GET_PIXEL_X;
            } else {
                state = NEUTRAL;
                invalidCommand();
            }
            buf.removeLeading(1);
            break;

        NEXT_STATE(GET_PIXEL_X, GET_PIXEL_Y)

        case GET_PIXEL_Y: {

            uint8_t x = buf[0];
            uint8_t y = buf[1];

            uint8_t replyData;
            if (!flipdotBuffer->isStateKnown()) {
                replyData = STATE_UNKNOWN;
            } else if (!flipdotBuffer->isPixelValid(x, y)) {
                replyData = ADDR_INVALID;
            } else {
                replyData = flipdotBuffer->getPixel(x, y);
            }

            revertCursor(replyData);
            state = GET_PIXEL_NEXT;
        }   break;

        case SET_INC_MODE:

            if (b > FlipdotDisplay::DisplayMode_Last) {
                respond(ADDR_INVALID);
            } else {
                bool success = flipdotBuffer->setDisplayMode((FlipdotDisplay::DisplayMode) b);
                respond(success ? ACK : BUFFER_OVERFLOW);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;

        case SET_FLIP_SPEED:

            if (cursor < 1) {
                cursor++;
            } else {
                uint8_t lo = buf[0];
                uint8_t hi = buf[1];
                uint16_t pixelsPerSecond = (hi << 8) | lo;

                bool success = flipdotBuffer->setFlipSpeed(pixelsPerSecond);
                revertCursor(success ? ACK : BUFFER_OVERFLOW);
            }
            break;

        case SET_LED_TRX_MODE:

            if (buf[0] > WS2812Controller::TransitionMode::SLIDE_QUICK ||
                buf[0] < WS2812Controller::TransitionMode::IMMEDIATE) {
                respond(ADDR_INVALID);
            } else {
                bool success = ledBuffer->setTransitionMode((WS2812Controller::TransitionMode) buf[0]);
                respond(success ? ACK : BUFFER_OVERFLOW);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;

        case SET_TRX_MODE:

            if (b > FlipdotDisplay::TransitionMode_Last) {
                respond(ADDR_INVALID);
            } else {
                bool success = flipdotBuffer->setTransitionMode((FlipdotDisplay::TransitionMode) b);
                respond(success ? ACK : BUFFER_OVERFLOW);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;
    }

    return true;
}

void CommandInterpreter::reset() {
    state = NEUTRAL;
    cursor = 0;
    buf.clear();
}

void CommandInterpreter::revertCursor() {
    revertCursor(0x00);
}

void CommandInterpreter::revertCursor(uint8_t response) {
    buf.removeLeading(cursor + 1);
    cursor = 0;
    state = NEUTRAL;
    if (response != 0x00) {
        respond(&response, 1);
    }
}

void CommandInterpreter::respond(const uint8_t* data, size_t len) {
    /*for (int i = 0; i < len; i++) {
        printf("%#04x ", data[i]);
    }
    printf("\n");*/
    if (responseHandler != nullptr) {
        responseHandler(data, len);
    }
}

void CommandInterpreter::ack() {
    respond(&ACK, 1);
}

void CommandInterpreter::invalidCommand() {
    respond(&INVALID_COMMAND, 1);
}

