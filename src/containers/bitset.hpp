#ifndef __CONTAINERS_BITSET_HPP__
#define __CONTAINERS_BITSET_HPP__

#include <stdint.h>

#include "utils2.hpp"

class bitset_t {
private:
    size_t _count, _size;
    std::vector<uint64_t> bits;

public:
    bitset_t() {
        _size = 0;
        _count = 0;
#ifndef NDEBUG
        verify();
#endif
    }

    explicit bitset_t(size_t size) {
        bits.resize(ceil_aligned(size, 64) / 64, 0);
        _size = size;
        _count = 0;
#ifndef NDEBUG
        verify();
#endif
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

    void resize(size_t size) {
        bits.resize(ceil_aligned(size, 64) / 64, 0);
        _size = size;
        _count = 0;
        for (int i = 0; i < (int)_size; i++) if (test(i)) _count++;
#ifndef NDEBUG
        verify();
#endif
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
        rassert(c == _count);
    }
#endif
};

#endif /* __CONTAINERS_BITSET_HPP__ */
