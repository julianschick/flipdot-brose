#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cstdlib>
#include <stdint.h>

/**
 * \brief Efficient circular buffer.
 *
 * Circular buffer implementation for working on incoming data streams without
 * having to reallocate heap space each time data comes in and with efficient
 * removal of leading and trailing bytes.
 *
 * The base data type is uint8_t, i.e. a normal unsigned byte.
 */
class RingBuffer {

public:
    /**
     * Construct a new instance (internally allocate size + 1 bytes of memory).
     *
     * Optionally, an out of bounds handler can be specified. It is called, whenever an index is accessed, that does
     * not exist. This routine could for instance log the invalid access or throw an exception. This way you can
     * choose by yourself, whether you want to use exceptions in your program.
     *
     * @param capacity Capacity in bytes.
     * @param outOfBoundsHandler Optionally, give a function pointer, which is called whenever an index is accessed,
     * that does not exist.
     */
    RingBuffer(size_t capacity, void (*outOfBoundsHandler)(size_t, size_t) = nullptr);

    /**
     * Destroy the instance (cleans up the internally allocated buffer memory).
     */
    ~RingBuffer();

    /**
     * @return Capacity of the buffer. That is, maximum amount of data it can hold.
     * @see getLength()
     * @see getFreeSpace()
     */
    inline size_t getCapacity() { return size - 1; };

    /**
     * @return Length of the buffer. That is, current amount of data in the buffer.
     * @see getCapacity()
     */
    size_t getLength();

    /**
     * @return Currently available free bytes in the buffer.
     * @see getCapacity()
     */
    inline size_t getFreeSpace();

    /**
     * Get byte.
     * In case of an invalid index, the out of bounds handler is called if provided and 0 is returned.
     * @param index Index of byte.
     * @return Byte value.
     * @see operator[]()
     */
    uint8_t at(size_t index);

    /**
     * Get byte.
     * In case of an invalid index, the out of bounds handler is called if provided.
     * @param index Index of byte.
     * @return Byte value.
     * @see at()
     */
    uint8_t operator[](size_t index);

    /**
     * Get range of bytes.
     * In case of an invalid index, the out of bounds handler is called if provided.
     * @param data Target buffer for bytes.
     * @param index Starting index of the range.
     * @param len Length of the range.
     */
    void at(uint8_t* data, size_t index, size_t len);

    /**
     * Append bytes if enough space for all of them is available.
     * @param data Pointer to bytes to be appended.
     * @param len Number of bytes to be appended.
     * @return true, if bytes could be appended, false if not.
     */
    bool append(uint8_t* data, size_t len);

    /**
     * Append one byte if space for it is available.
     * @param data Byte to be appended.
     * @return true, if byte could be appended, false otherwise.
     */
    bool append(uint8_t data);

    /**
     * Append as many bytes as possible.
     * @param data Pointer to bytes to be appended.
     * @param len Number of bytes to be appended.
     * @return Number of actually appended bytes.
     */
    size_t appendIfPossible(uint8_t* data, size_t len);

    /**
     * Remove leading bytes.
     * @param len Number of bytes to be removed.
     * @return Number of actually removed bytes.
     */
    size_t removeLeading(size_t len);

    /**
     * Remove trailing bytes.
     * @param len Number of bytes to be removed.
     * @return Number of actually removed bytes.
     */
    size_t removeTrailing(size_t len);

    /**
     * Clear buffer
     * @return  Number of removed bytes.
     */
    size_t clear();

private:
    uint8_t* buf;
    size_t size;

    size_t head = 0;
    size_t tail = 0;

    void (*outOfBoundsHandler) (size_t index, size_t maxIndex);

};


#endif //RINGBUFFER_H
