#include "ringbuffer.h"

#include <algorithm>

RingBuffer::RingBuffer(size_t capacity, void (*outOfBoundsHandler)(size_t, size_t)) :
    size(capacity + 1),
    outOfBoundsHandler(outOfBoundsHandler) {

    buf = new uint8_t[capacity];
}

RingBuffer::~RingBuffer() {
    delete[] buf;
}

size_t RingBuffer::getLength() {
    if (head <= tail) {
        return tail - head;
    } else {
        return tail + size - head;
    }
}

inline size_t RingBuffer::getFreeSpace() {
    return size - getLength() - 1;
}

uint8_t RingBuffer::at(size_t index) {
    if (index < getLength()) {
        return buf[(head + index) % size];

    } else if (outOfBoundsHandler != nullptr) {
        outOfBoundsHandler(index, getLength() - 1);
    }

    return 0;
}

uint8_t RingBuffer::operator[](size_t index) {
    return at(index);
}

void RingBuffer::at(uint8_t *data, size_t index, size_t len) {
    if (index + len < getLength()) {

        for (size_t i = 0; i < len; i++) {
            data[i] = buf[(head + index + i) % size];
        }

    } else if (outOfBoundsHandler != nullptr) {
        outOfBoundsHandler(index + len, getLength() - 1);
    }

}

bool RingBuffer::append(uint8_t *data, size_t len) {
    if (getFreeSpace() < len) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        buf[(tail + i) % size] = data[i];
    }

    tail = (tail + len) % size;
    return true;
}

bool RingBuffer::append(uint8_t data) {
    if (getFreeSpace() < 1) {
        return false;
    }

    buf[tail] = data;
    tail = (tail + 1) % size;
    return true;
}

size_t RingBuffer::appendIfPossible(uint8_t *data, size_t len) {
    size_t actuallyAppended = std::min(len, getFreeSpace());

    for (size_t i = 0; i < actuallyAppended; i++) {
        buf[(tail + i) % size] = data[i];
    }

    tail = (tail + actuallyAppended) % size;
    return actuallyAppended;
}

size_t RingBuffer::removeLeading(size_t len) {
    size_t actuallyRemoved = std::min(len, getLength());

    head = (head + actuallyRemoved) % size;
    return actuallyRemoved;
}

size_t RingBuffer::removeTrailing(size_t len) {
    size_t actuallyRemoved = std::min(len, getLength());

    tail = (tail - actuallyRemoved + size) % size;
    return actuallyRemoved;
}

size_t RingBuffer::clear() {
    size_t actuallyRemoved = getLength();

    head = 0;
    tail = 0;
    return actuallyRemoved;
}
