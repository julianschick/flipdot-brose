#include "bitarray.h"

#include <esp_log.h>
#include <algorithm>
#include <cstring>

BitArray::BitArray(int size_) {
    size = size_;
    data_size = (size / 32) + (size % 32 ? 1 : 0);
    data = new uint32_t[data_size];

    setAll(false);

    uint32_t mask = 1;
    for (int i = 0; i < 32; i++) {
        masks[i] = mask;
        mask <<= 1;
    }
}

BitArray::BitArray(const BitArray &other) : BitArray(other.size) {
    for (int i = 0; i < data_size; i++) {
        data[i] = other.data[i];
    }
}

BitArray::~BitArray() {
    delete[] data;
}

bool BitArray::get(int index) const {
    if (!range_check(index)) {
        return false;
    }

    return data[index / 32] & masks[index % 32];
}

void BitArray::set(int index, bool value) {
    if (!range_check(index)) {
        return;
    }

    int block_index = index / 32;
    int bit_index = index % 32;

    if (value) {
        data[block_index] |= (uint32_t) masks[bit_index];
    } else {
        data[block_index] &= ~masks[bit_index];
    }
}

void BitArray::set8(int index, uint8_t value) {
    if (!range_check(index) || !range_check(index + 7)) {
        return;
    }

    for (int i = 0; i < 8; i++) {
        set(index + i, value & 0x01);
        value >>= 1;
    }
}


void BitArray::setAll(bool value) {
    for (int i = 0; i < data_size; i++) {
        data[i] = value ? 0xFFFFFFFF : 0;
    }
}

void BitArray::copy_from(const BitArray& other) {
    if (size == other.size) {
        xthal_memcpy(data, other.data, data_size * 4);
    } else {
        int copy_size = std::min(size, other.size);

        for (int i = 0; i < copy_size; i++) {
            set(i, other.get(i));
        }
    }
}

void BitArray::copy_from(uint8_t* buffer, size_t len) {
    size_t copy_len = MIN(len, data_size * 4);

    xthal_memcpy(data, buffer, copy_len);
}

bool BitArray::transition_vector_to(const class BitArray & other, int8_t* transition_vector) {
    if (size != other.size) {
        return false;
    }

    memset(transition_vector, 0, size);

    for (int i = 0; i < data_size; i++) {
        if (data[i] != other.data[i]) {
            uint32_t to_reset = data[i] & ~other.data[i];
            uint32_t to_set = ~data[i] & other.data[i];

            for (int j = 0; j < 32; j++) {
                if (to_reset & 0x01) {
                    transition_vector[i * 32 + j] = -1;
                } else if (to_set & 0x01) {
                    transition_vector[i * 32 + j] = +1;
                }
                to_reset >>= 1;
                to_set >>= 1;
            }
        }
    }

    return true;
}

bool BitArray::range_check(int index) const {
    bool ok = index >= 0 && index < size;

    if (!ok) {
        ESP_LOGW(TAG_BITARRAY, "BitArray index out of bounds: %d.", index);
    }

    return ok;
}