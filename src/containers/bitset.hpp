#ifndef __CONTAINERS_BITSET_HPP__
#define __CONTAINERS_BITSET_HPP__

class bitset_t {

private:
    size_t _size, _count;
    uint64_t *bits;

public:
    bitset_t(size_t size) {
        bits = (uint64_t*)malloc(ceil_aligned(size, 64) / 64 * sizeof(uint64_t));
        bzero(bits, ceil_aligned(size, 64) / 64 * sizeof(uint64_t));
        _size = size;
        _count = 0;
    }
    ~bitset_t() {
        free(bits);
    }

public:
    bool operator[](unsigned int place) const {
        return test(place);
    }
    
    bool test(unsigned int place) const {
        assert(place < size());
        return bits[place / 64] & (1 << (place % 64));
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
            bits[place / 64] |= (1 << (place % 64));
        } else {
            if (test(place)) _count--;
            bits[place / 64] &= ~ (1 << (place % 64));
        }
    }
    
    size_t size() const {
        return _size;
    }
    
    size_t count() const {
        return _count;
    }
};

#endif /* __CONTAINERS_BITSET_HPP__ */
