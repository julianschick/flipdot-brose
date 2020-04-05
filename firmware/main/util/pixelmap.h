#ifndef COORDMAP_H
#define COORDMAP_H

#include <stddef.h> //size_t

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

    size_t get_width() { return width; };
    size_t get_height() { return height; };

private:
    size_t width;
    size_t height;
    bool column_wise;

};


#endif //COORDMAP_H
