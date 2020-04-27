#ifndef FLIPDOT_PIXELSTRING_H
#define FLIPDOT_PIXELSTRING_H

#include <string>
#include "bitarray.h"
#include "pixelmap.h"
#include "../font/pixelfont.h"

using namespace std;

class PixelString {

public:
    enum TextAlignment {
        LEFT        = 0x00,
        RIGHT       = 0x01,
        CENTERED    = 0x02
    };
    static const TextAlignment TextAlignment_Last = CENTERED;

public:
    PixelString(string& str);

    void print(BitArray& target, PixelMap& pixelmap, PixelFont& regular_font, PixelFont& bold_font, TextAlignment text_alignment);

private:
    string& str;

};


#endif //FLIPDOT_PIXELSTRING_H
