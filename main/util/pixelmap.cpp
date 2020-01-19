#include "pixelmap.h"

PixelMap::PixelMap(size_t width, size_t height, bool column_wise)
    : width(width), height(height), column_wise(column_wise) { }

size_t PixelMap::index(size_t x, size_t y) {
    if (column_wise) {
        return x * height + y;
    } else {
        return y * width + x;
    }
}

size_t PixelMap::index(PixelCoord& coords) {
    return index(coords.x, coords.y);
}

void PixelMap::coords(size_t index, size_t& x, size_t& y) {
    if (column_wise) {
        x = index / height;
        y = index % height;
    } else {
        x = index % width;
        y = index / width;
    }
}

PixelCoord PixelMap::coords(size_t index) {
    PixelCoord c;
    coords(index, c.x, c.y);
    return c;
};
