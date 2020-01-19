#include "bitarray.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <iostream>
#include <esp_log.h>

BitArray::BitArray(int size) : size(size) {
    data_size_32 = (size / 32) + (size % 32 ? 1 : 0);
    data_size_8 = (size / 8) + (size % 8 ? 1 : 0);
    data_ptr_32 = new uint32_t[data_size_32];
    data_ptr_8 = (uint8_t*) data_ptr_32;

    last_mask_32 = ~(0xFFFFFFFF << (size % 32));
    last_mask_8 = ~(0xFF << (size % 8));

    reset();
}

BitArray::BitArray(const BitArray &other) : BitArray(other.size) {
    xthal_memcpy(data_ptr_8, other.data_ptr_8, other.data_size_8);
}

BitArray::~BitArray() {
    delete[] data_ptr_32;
}

bool BitArray::operator[](size_t index) const {
    return get(index);
}

bool BitArray::get(size_t index) const {
    if (!range_check(index)) {
        return false;
    }

    return data_ptr_32[index / 32] & (1 << (index % 32));
}

bool BitArray::all() {
    bool result = true;

    for (size_t i = 0; i < data_size_32 && result; i++) {
        result = result && (~data_ptr_32[i] & mask32(i)) == 0;
    }

    return result;
}

bool BitArray::none() {
    bool result = true;

    for (size_t i = 0; i < data_size_32 && result; i++) {
        result = result && (data_ptr_32[i] & mask32(i)) == 0;
    }

    return result;
}

bool BitArray::any() {
    bool result = false;

    for (size_t i = 0; i < data_size_32 && !result; i++) {
        result = result || (data_ptr_32[i] & mask32(i)) != 0;
    }

    return result;
}

void BitArray::set() {
    set_(true);
}

void BitArray::set(size_t index) {
    set(index, true);
}

void BitArray::set(size_t index, bool value) {
    if (!range_check(index)) {
        return;
    }

    int block_index = index / 32;
    int bit_index = index % 32;

    if (value) {
        data_ptr_32[block_index] |= (1 << bit_index);
    } else {
        data_ptr_32[block_index] &= ~(1 << bit_index);
    }
}

void BitArray::set_(bool value) {
    for (size_t i = 0; i < data_size_32; i++) {
        if (value) {
            data_ptr_32[i] = 0xFFFFFFFF & mask32(i);
        } else {
            data_ptr_32[i] = 0;
        }
    }
}

void BitArray::reset() {
    set_(false);
}

void BitArray::reset(size_t index) {
    set(index, false);
}

void BitArray::flip() {
    for (size_t i = 0; i < data_size_32; i++) {
        data_ptr_32[i] = ~data_ptr_32[i] & mask32(i);
    }

}

void BitArray::flip(size_t index) {
    if (!range_check(index)) {
        return;
    }

    set(index, !get(index));
}

void BitArray::set8(size_t index, uint8_t value) {
    if (!range_check(index) || !range_check(index + 7)) {
        return;
    }

    if (index % 8 == 0) {
        data_ptr_8[index / 8] = value;
    } else {
        int shift = index % 8;
        size_t block = index / 8;

        uint8_t lower_mask = 0xFF << shift;
        uint8_t higher_mask = 0xFF >> (8 - shift);

        data_ptr_8[block] = data_ptr_8[block] | ((value << shift) & lower_mask);
        data_ptr_8[block] = data_ptr_8[block] & ((value << shift) | ~lower_mask);

        data_ptr_8[block+1] = data_ptr_8[block+1] | ((value >> (8 - shift)) & higher_mask);
        data_ptr_8[block+1] = data_ptr_8[block+1] & ((value >> (8 - shift)) | ~higher_mask);
    }

}

void BitArray::copy_from(const BitArray& other) {
    if (size == other.size) {
        xthal_memcpy(data_ptr_8, other.data_ptr_8, data_size_8);
    } else {

        if (size < other.size && size > 0) {
            xthal_memcpy(data_ptr_8, other.data_ptr_8, data_size_8);
        }

        if (size > other.size && other.size > 0) {
            size_t last = other.data_size_8 - 1;

            if (other.data_size_8 > 1) {
                xthal_memcpy(data_ptr_8, other.data_ptr_8, last);
            }
            data_ptr_8[last] = data_ptr_8[last] | (other.data_ptr_8[last] & other.mask8(last));
            data_ptr_8[last] = data_ptr_8[last] & (other.data_ptr_8[last] | ~other.mask8(last));
        }
    }
}

void BitArray::copy_from(uint8_t* buffer, size_t bytes) {
    size_t copy_len = std::min(bytes, data_size_8);

    xthal_memcpy(data_ptr_8, buffer, copy_len);
}

bool BitArray::transition_vector_to(const class BitArray& other, BitArray& set_vector, BitArray& reset_vector) {
    set_vector.reset();
    reset_vector.reset();

    if (size != other.size || size != set_vector.size || size != reset_vector.size) {
        return false;
    }

    for (int i = 0; i < data_size_32; i++) {
        if (data_ptr_32[i] != other.data_ptr_32[i]) {
            reset_vector.data_ptr_32[i] = data_ptr_32[i] & ~other.data_ptr_32[i];
            set_vector.data_ptr_32[i] = ~data_ptr_32[i] & other.data_ptr_32[i];
        }
    }

    return true;
}

std::string BitArray::to_string() const {
    std::stringstream stream;

    for (size_t i = 0; i < size; i++) {
        stream << get(i);
    }

    return stream.str();
}

inline uint32_t BitArray::mask32(size_t index32) const {
    if (index32 < data_size_32 - 1 || size % 32 == 0) {
        return 0xFFFFFFFF;
    } else {
        return last_mask_32;
    }
}

inline uint8_t BitArray::mask8(size_t index8) const {
    if (index8 < data_size_8 - 1 || size % 8 == 0) {
        return 0xFF;
    } else {
        return last_mask_8;
    }
}

bool BitArray::range_check(int index) const {
    bool ok = index >= 0 && index < size;

    if (!ok) {
        ESP_LOGW(TAG_BITARRAY, "BitArray index out of bounds: %d.", index);
    }

    return ok;
}
