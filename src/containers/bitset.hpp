#ifndef CONTAINERS_BITSET_HPP_
#define CONTAINERS_BITSET_HPP_

#include <stdint.h>

#include <vector>

#include "utils.hpp"

class bitset_t {
private:
    size_t _count, _size;
    std::vector<uint64_t> bits;

    void set_without_updating_count(unsigned int place, bool value) {
        rassert(place < size());
        if (value) {
            bits[place / 64] |= (uint64_t(1) << (place % 64));
        } else {
            bits[place / 64] &= ~(uint64_t(1) << (place % 64));
        }
    }

public:
    bitset_t() {
        _size = 0;
        _count = 0;
    }

    explicit bitset_t(size_t size) {
        bits.resize(ceil_aligned(size, 64) / 64, 0);
        _size = size;
        _count = 0;
    }

public:
    bool operator[](unsigned int place) const {
        return test(place);
    }

    bool test(unsigned int place) const {
        rassert(place < size());
        return bits[place / 64] & (uint64_t(1) << (place % 64));
    }

    void set() {
        for (unsigned int i = 0; i < size(); i++) {
            set(i);
        }
    }

    void set(unsigned int place, bool value = true) {
        rassert(place < size());
        if (value) {
            if (!test(place)) _count++;
        } else {
            if (test(place)) _count--;
        }
        set_without_updating_count(place, value);
    }

    size_t size() const {
        return _size;
    }

    void reserve(size_t size) {
        bits.reserve(ceil_aligned(size, 64) / 64);
    }

    void resize(size_t new_size, bool value = false) {

        size_t old_size = _size;

        /* Update the count to no longer include any bits that we will chop off */
        for (size_t i = new_size; i < old_size; i++) {
            if (test(i)) _count--;
        }

        /* Actually resize the bitset */
        bits.resize(ceil_aligned(new_size, 64) / 64, value ? 0xFFFFFFFFFFFFFFFF : 0);
        _size = new_size;

        /* `std::vector::resize()` correctly initialized any new chunks, but we must correctly set
        the upper bits of the highest old chunk. For example, suppose that we grow from size 60 to
        68 and `value` is `true`. A new 64-bit chunk will be added, and it will contain all 1s,
        which is correct. However, the contents of the top 4 bits of the old 64-bit chunk were
        garbage and we don't know what their values are, so we must manually set them. */
        for (size_t i = old_size; i < new_size && i % 64 != 0; i++) {
            set_without_updating_count(i, value);
        }

        /* Update the count to take into account any new bits we set */
        if (value && new_size > old_size) _count += new_size - old_size;
    }

    size_t count() const {
        return _count;
    }

    /* Just for debugging; makes sure that `_count` is in agreement with the actual bit
    values */
    void verify() const {
        unsigned c = 0;
        for (unsigned i = 0; i < _size; i++) {
            if (test(i)) c++;
        }
        rassert(c == _count);
    }
};

#endif /* CONTAINERS_BITSET_HPP_ */
