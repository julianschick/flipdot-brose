#include "commandinterpreter.h"

#include <string>
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

            if (b == 0xE0) {

                uint32_t wm = uxTaskGetStackHighWaterMark(ledTask);
                printf("led-task watermark = %d\n", wm);

                wm = uxTaskGetStackHighWaterMark(tcpServerTask);
                printf("tcp-task watermark = %d\n", wm);

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
                dsp->clear();
                respond(ACK);
                buf.removeLeading(1);

            // fill display
            } else if (b == 0x92) {
                dsp->fill();
                respond(ACK);
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

                if (!dsp->is_state_known()) {
                    respond(&STATE_UNKNOWN, 1);
                } else {
                    uint8_t replyData[(dsp->get_number_of_pixels() / 8) + 1];
                    size_t replyLen = dsp->copy_state(&replyData[0], sizeof(replyData));
                    respond(&replyData[0], replyLen);
                }

            // get pixel
            } else if (b == 0xB1) {
                buf.removeLeading(1);
                state = GET_PIXEL_X;

            // get all leds
            } else if (b == 0xC0) {
                buf.removeLeading(1);

                color_t *data = led_ctrl->getAll();
                uint8_t replyData[led_ctrl->getLedCount() * 3];

                for (size_t i = 0; i < led_ctrl->getLedCount(); i++) {
                    replyData[i * 3 + 0] = data[i].brg.red;
                    replyData[i * 3 + 1] = data[i].brg.green;
                    replyData[i * 3 + 2] = data[i].brg.blue;
                }

                respond(&replyData[0], sizeof(replyData));

            // set pixel incremental mode
            } else if (b == 0xD0) {
                buf.removeLeading(1);
                state = SET_INC_MODE;

            // set pixel overlay mode
            } else if (b == 0xD1) {
                buf.removeLeading(1);
                state = SET_OVERLAY_MODE;

            // set led transition mode
            } else if (b == 0xD2) {
                buf.removeLeading(1);
                state = SET_LED_TRX_MODE;

            // set transition mode
            } else if (b == 0xD3) {
                buf.removeLeading(1);
                state = SET_TRX_MODE;

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
                BitArray bitset (dsp->get_number_of_pixels());
                bitset.copy_from(&receivedBytes[0], len);
                dsp->display(bitset);

                revertCursor(ACK);
            }
            break;

        NEXT_STATE(SHOW_TEXT_LEN_ALIGN, SHOW_TEXT_CHARS)

        case SHOW_TEXT_CHARS:
            len = (buf[1] & 0xFC) >> 2;

            if (cursor - 1 < len) {
                cursor++;
            } else {
                PixelString::TextAlignment alignment = toAlignment(buf[1]);

                uint8_t receivedBytes[len];
                for (size_t i = 0; i < len; i++) {
                    receivedBytes[i] = buf[i + 2];
                }

                revertCursor(ACK);

                std::string str = std::string((char*)&receivedBytes[0], len);
                ESP_LOGI(TAG, "string  of len %d received = %s", len, str.c_str());
                dsp->display_string(str, alignment);
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
                revertCursor(INVALID_COMMAND);
            }
            break;

        NEXT_STATE(SET_PIXEL_X, SET_PIXEL_Y);

        case SET_PIXEL_Y: {
            bool show = buf[0];
            uint8_t x = buf[1];
            uint8_t y = buf[2];

            ESP_LOGI(TAG, "(%d, %d) = %d", x, y, show);
            dsp->flip_single_pixel(x, y, show);

            revertCursor(ACK);
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

                bool success = led_ctrl->setAllLedsToSameColor(c);

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
            ESP_LOGV(TAG, "set_led_next, current = %d, cursor = %d", b, cursor);

            if (b == STOP) {
                ESP_LOGV(TAG, "stopped, cursor = %d", cursor);

                if (cursor > 0) {
                    std::vector<WS2812Controller::LedChangeCommand>* cmds =
                            new std::vector<WS2812Controller::LedChangeCommand>(cursor / 4);

                    for (size_t i = 0; i < cursor / 4; i++) {
                        uint8_t addr = 0x7F & buf[i*4 + 0];

                        if (addr < led_ctrl->getLedCount()) {
                            color_t c = {{
                                 (uint8_t) buf[i*4 + 3],
                                 (uint8_t) buf[i*4 + 1],
                                 (uint8_t) buf[i*4 + 2],
                                 0x00
                             }};

                            WS2812Controller::LedChangeCommand cmd = {addr, c};
                            cmds->push_back(cmd);
                        }
                    }

                    bool success = led_ctrl->setLeds(cmds);
                    revertCursor(success ? ACK : BUFFER_OVERFLOW);
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
            ESP_LOGV(TAG, "cursor now %d", cursor);
            if (cursor % 4 == 0) {
                ESP_LOGV(TAG, "state led next");
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
            if (!dsp->is_state_known()) {
                replyData = STATE_UNKNOWN;
            } else if (!dsp->is_valid_index(x, y)) {
                replyData = ADDR_INVALID;
            } else {
                replyData = dsp->get_pixel(x, y);
            }

            respond(&replyData, 1);
            revertCursor(0x00);
            state = GET_PIXEL_NEXT;
        }   break;

        case SET_INC_MODE:

            if (b > FlipdotDisplay::INCREMENTAL) {
                respond(ADDR_INVALID);
            } else {
                dsp->setDisplayMode((FlipdotDisplay::DisplayMode) b);
                respond(ACK);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;

        case SET_OVERLAY_MODE:
            buf.removeLeading(1);

            if (b > FlipdotDisplay::NEGATIVE) {
                respond(ADDR_INVALID);
            } else {
                dsp->setOverlayMode((FlipdotDisplay::OverlayMode) b);
                respond(ACK);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;


        case SET_LED_TRX_MODE:

            if (buf[0] > WS2812Controller::TransitionMode::SLIDE_QUICK ||
                buf[0] < WS2812Controller::TransitionMode::IMMEDIATE) {
                respond(ADDR_INVALID);
            } else {
                bool success = led_ctrl->setTransitionMode((WS2812Controller::TransitionMode) buf[0]);
                respond(success ? ACK : BUFFER_OVERFLOW);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            break;

        case SET_TRX_MODE:

            if (b > FlipdotDisplay::BOTTOM_UP) {
                respond(ADDR_INVALID);
            } else {
                dsp->setTransitionMode((FlipdotDisplay::TransitionMode) b);
                respond(ACK);
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

