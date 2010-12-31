#ifndef __CONTAINERS_BITSET_HPP__
#define __CONTAINERS_BITSET_HPP__

#include <stdint.h>

#include "utils2.hpp"

class bitset_t {
private:
    size_t _size, _count;
    uint64_t *bits;

public:
    explicit bitset_t(size_t size) {
        bits = new uint64_t[ceil_aligned(size, 64) / 64];
        bzero(bits, ceil_aligned(size, 64) / 64 * sizeof(uint64_t));
        _size = size;
        _count = 0;
#ifndef NDEBUG
        verify();
#endif
    }
    ~bitset_t() {
        delete[] bits;
    }

public:
    bool operator[](unsigned int place) const {
        return test(place);
    }

    bool test(unsigned int place) const {
        assert(place < size());
        return bits[place / 64] & (uint64_t(1) << (place % 64));
    }

    void set() {
        for (unsigned int i = 0; i < size(); i++) {
            set(i);
        }
    }

    void set(unsigned int place, bool value = true) {
        assert(place < size());
        if (value) {
            if (!test(place)) _count++;
            bits[place / 64] |= (uint64_t(1) << (place % 64));
        } else {
            if (test(place)) _count--;
            bits[place / 64] &= ~ (uint64_t(1) << (place % 64));
        }
#ifndef NDEBUG
        verify();
#endif
    }

    size_t size() const {
        return _size;
    }

    size_t count() const {
        return _count;
    }

#ifndef NDEBUG
    void verify() const {
        unsigned c = 0;
        for (unsigned i = 0; i < _size; i++) {
            if (test(i)) c++;
        }
        assert(c == _count);
    }
#endif
};

#endif /* __CONTAINERS_BITSET_HPP__ */
