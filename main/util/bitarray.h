#ifndef BITARRAY_H
#define BITARRAY_H

#include "../globals.h"

class BitArray {

public:
    BitArray(int size);
    BitArray(const BitArray &other);
    ~BitArray();

    bool operator[](size_t index) const;
    bool get(size_t index) const;

    bool all();
    bool none();
    bool any();

    void set();
    void set(size_t index);
    void set(size_t index, bool value);

    void reset();
    void reset(size_t index);

    void flip();
    void flip(size_t index);

    void set8(size_t index, uint8_t value);

    void copy_from(const BitArray& other);
    void copy_from(uint8_t* buffer, size_t bytes);

    bool transition_vector_to(const class BitArray& other, BitArray& set_vector, BitArray& reset_vector);

    std::string to_string() const;

private:
    size_t size;
    size_t data_size_32;
    size_t data_size_8;

    uint32_t* data_ptr_32;
    uint8_t* data_ptr_8;
    uint32_t last_mask_32;
    uint8_t last_mask_8;

    void set_(bool value);
    inline uint32_t mask32(size_t index) const;
    inline uint8_t mask8(size_t index) const;
    inline bool range_check(int index) const;

};


#endif //BITARRAY_H
