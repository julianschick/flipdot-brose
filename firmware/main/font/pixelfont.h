#ifndef FLIPDOT_PIXELFONT_H
#define FLIPDOT_PIXELFONT_H


class PixelFont {

public:
    virtual ~PixelFont() { }

    bool has_char(char c) {
        return get_jumps()[(int) c] != -1;
    }

    uint8_t get_width(char c) {
        return get_widths()[(int) c];
    };

    uint8_t get_octet(char c, uint8_t index) {
        size_t jump_index = get_jumps()[(int) c];

        if (index >= get_width(c)) {
            return 0x00;
        }

        return get_chars()[jump_index + index];
    }

private:
    virtual const uint8_t* get_chars() = 0;
    virtual const int* get_jumps() = 0;
    virtual const int* get_widths() = 0;

};

#endif //FLIPDOT_PIXELFONT_H
