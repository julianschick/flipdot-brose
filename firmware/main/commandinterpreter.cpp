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
    FlipdotDisplay::DisplayMode displayMode = FlipdotDisplay::OVERRIDE;

    switch (state) {
        case NEUTRAL: // cursor == 0

            if (b == WHOAMI) {
                uint8_t r[2] = {0x70, 0x07};
                respond(&r[0], sizeof(r));
                buf.removeLeading(1);

            // show bitset
            } else if (b == 0x90) {
                state = SHOW_BITSET_INC;
                cursor++;

            // clear display
            } else if (b == 0x91) {
                state = CLEAR_DISPLAY_INC;
                buf.removeLeading(1);

            // fill display
            } else if (b == 0x92) {
                state = FILL_DISPLAY_INC;
                buf.removeLeading(1);

            // show text message
            } else if (b == 0x93) {
                state = SHOW_TEXT_INC;
                cursor++;

            // set pixel
            } else if (b == 0x94) {
                state = SET_PIXEL_INC;
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

                uint8_t replyData[led_drv->get_led_count() * 3];

                for (size_t i = 0; i < led_drv->get_led_count(); i++) {
                    color_t color = led_drv->get_color(i);
                    replyData[i*3 + 0] = color.brg.red;
                    replyData[i*3 + 1] = color.brg.green;
                    replyData[i*3 + 2] = color.brg.blue;
                }

                respond(&replyData[0], sizeof(replyData));

            // close connection
            } else if (b == 0xD0) {
                connectionCloseHandler();
                reset();
                return false;

            // invalid command
            } else {
                ESP_LOGE(TAG, "invalid command %#04x\n", b);
                buf.removeLeading(1);
                invalidCommand();
            }

            break;

        NEXT_STATE(SHOW_BITSET_INC, SHOW_BITSET_LEN)
        NEXT_STATE(SHOW_BITSET_LEN, SHOW_BITSET_BITS)

        case SHOW_BITSET_BITS:
            len = buf[2];

            if (cursor - 2 < len) {
                cursor++;
            } else {
                displayMode = toDisplayMode(buf[1]);

                uint8_t receivedBytes[len];
                for (size_t i = 0; i < len; i++) {
                    receivedBytes[i] = buf[i+3];
                }
                BitArray bitset (dsp->get_number_of_pixels());
                bitset.copy_from(&receivedBytes[0], len);
                dsp->display(bitset, displayMode);

                revertCursor(ACK);
            }
            break;

        case CLEAR_DISPLAY_INC:
            displayMode = toDisplayMode(buf[0]);

            if (displayMode == FlipdotDisplay::OVERRIDE) {
                dsp->clear();
            } else {
                BitArray blank (dsp->get_number_of_pixels());
                blank.reset();
                dsp->display(blank, displayMode);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            respond(&ACK, 1);
            break;

        case FILL_DISPLAY_INC:
            displayMode = toDisplayMode(buf[0]);

            if (displayMode == FlipdotDisplay::OVERRIDE) {
                dsp->fill();
            } else {
                BitArray filled (dsp->get_number_of_pixels());
                filled.set();
                dsp->display(filled, displayMode);
            }

            buf.removeLeading(1);
            state = NEUTRAL;
            respond(&ACK, 1);
            break;

        NEXT_STATE(SHOW_TEXT_INC, SHOW_TEXT_LEN_ALIGN)
        NEXT_STATE(SHOW_TEXT_LEN_ALIGN, SHOW_TEXT_CHARS)

        case SHOW_TEXT_CHARS:
            len = (buf[2] & 0xFC) >> 2;

            if (cursor - 2 < len) {
                cursor++;
            } else {
                displayMode = toDisplayMode(buf[1]);
                PixelString::TextAlignment alignment = toAlignment(buf[2]);

                uint8_t receivedBytes[len];
                for (size_t i = 0; i < len; i++) {
                    receivedBytes[i] = buf[i + 3];
                }

                std::string str = std::string(receivedBytes[0], len);
                dsp->display_string(str, alignment, displayMode);

                revertCursor(ACK);
            }
            break;

        case SET_PIXEL_INC:
            displayMode = toDisplayMode(buf[0]);
            buf.removeLeading(1);
            state = SET_PIXEL_NEXT;
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
            dsp->flip_single_pixel(x, y, show, displayMode);

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
                     0x00
                 }};

                led_drv->set_all_colors(c);
                led_drv->update();

                revertCursor(ACK);
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
            if (buf[0] == STOP) {
                state = NEUTRAL;
                buf.removeLeading(1);
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
            if (cursor < 3) {
                cursor++;
            } else {

                uint8_t addr = 0x7F & buf[0];

                if (addr < led_drv->get_led_count()) {
                    color_t c = {{
                         (uint8_t) buf[3],
                         (uint8_t) buf[1],
                         (uint8_t) buf[2],
                         0x00
                    }};

                    led_drv->set_color(addr, c);
                    led_drv->update();

                    revertCursor(ACK);
                } else {
                    revertCursor(ADDR_INVALID);
                }


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

        case GET_PIXEL_Y:

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

