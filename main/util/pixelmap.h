#ifndef COORDMAP_H
#define COORDMAP_H

#include "../globals.h"

typedef struct {
    size_t x;
    size_t y;
} PixelCoord;

class PixelMap {

public:
    PixelMap(size_t width, size_t height, bool column_wise);

    size_t index(size_t x, size_t y);
    size_t index(PixelCoord& coords);
    void coords(size_t index, size_t& x, size_t& y);
    PixelCoord coords(size_t index);

private:
    size_t width;
    size_t height;
    bool column_wise;

};


#endif //COORDMAP_H
