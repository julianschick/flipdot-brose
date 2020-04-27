#ifndef COMMANDINTERPRETER_H
#define COMMANDINTERPRETER_H

#include "classes.h"
#include "ringbuffer.h"

#define NEXT_STATE(a, b) case a: state = b; cursor++; break;

class CommandInterpreter {

public:
    CommandInterpreter(size_t buffer_size, void (*responseHandler) (const uint8_t*, size_t), void (*connectionCloseHandler) ());
    ~CommandInterpreter();

    bool queue(uint8_t* data, size_t len);
    bool process();
    void reset();

private:
    enum State {
        NEUTRAL,

        SHOW_BITSET_LEN, SHOW_BITSET_BITS,
        SHOW_TEXT_LEN_ALIGN, SHOW_TEXT_CHARS,
        SET_PIXEL_NEXT, SET_PIXEL_X, SET_PIXEL_Y,

        SET_ALL_LEDS, SET_ALL_LEDS_NEXT,
        SET_LED_NEXT, SET_LED_RGB,

        GET_PIXEL_NEXT, GET_PIXEL_X, GET_PIXEL_Y,

        SET_INC_MODE,
        SET_FLIP_SPEED,
        SET_LED_TRX_MODE,
        SET_TRX_MODE
    };

    RingBuffer& buf;
    State state = NEUTRAL;
    size_t cursor = 0;

    void (*responseHandler) (const uint8_t* data, size_t len);
    void (*connectionCloseHandler) ();

    const uint8_t ACK = 0x80;
    const uint8_t WHOAMI = 0x81;
    const uint8_t STOP = 0x82;
    const uint8_t INVALID_COMMAND = 0x83;
    const uint8_t CONTINUE = 0x84;
    const uint8_t  STATE_UNKNOWN = 0x85;
    const uint8_t  ADDR_INVALID = 0x86;
    const uint8_t BUFFER_OVERFLOW = 0x87;


    inline bool toAlignment(uint8_t byte, PixelString::TextAlignment* alignment) {
        uint8_t code = byte & 0x03;

        if (code < PixelString::TextAlignment_Last) {
            *alignment = (PixelString::TextAlignment) code;
            return true;
        }

        return false;
    }

    // state machine flow
    void revertCursor(uint8_t response);

    void respond(const uint8_t* data, size_t len);
    inline void respond(const uint8_t response) { respond(&response, 1); };
    void ack();
    void invalidCommand();

    static void outOfBoundHandler(size_t index, size_t maxIndex);

    //
    struct color {
        uint8_t r,g,b;
    };

    color leds[6];
    bool pixels[16];
};


#endif //COMMANDINTERPRETER_H
