
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <vector>
#include <endian.h>
#include <arpa/inet.h>
#include "corefwd.hpp"
#include "errors.hpp"
#include "config/alloc.hpp"

void print_hd(void *buf, size_t offset, size_t length);

int get_cpu_count();
long get_available_ram();
long get_total_ram();

void *malloc_aligned(size_t size, size_t alignment = 64);

// Fast string compare
int sized_strcmp(const char *str1, int len1, const char *str2, int len2);

static inline void swap(void **x, void **y) {
    void *tmp = *x;
    *x = *y;
    *y = tmp;
}

// Buffer
template <int _size>
struct buffer_base_t
{
    char buf[_size];
    static const int size = _size;
};

template <int _size>
struct buffer_t : public buffer_base_t<_size>,
                  public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, buffer_t<_size> >
{
};


template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator first, ForwardIterator last,
                       StrictWeakOrdering comp) {
    for(ForwardIterator it = first; it + 1 < last; it++) {
        if (!comp(*it, *(it+1)))
            return false;
    }
    return true;
}

// Extend STL less a bit
namespace std {
    //Scamped from:
    //https://sdm.lbl.gov/fastbit/doc/html/util_8h_source.html
    // specialization of less<> to work with char*
    template <> struct less< char* > {
        bool operator()(const char*x, const char*y) const {
            return strcmp(x, y) < 0;
        }
    };

    // specialization of less<> on const char* (case sensitive comparison)
    template <> struct less< const char* > {
        bool operator()(const char* x, const char* y) const {
            return strcmp(x, y) < 0;
        }
    };
};

//network conversion
inline uint16_t ntoh(uint16_t val) { return be16toh(val); }
inline uint32_t ntoh(uint32_t val) { return be32toh(val); }
inline uint64_t ntoh(uint64_t val) { return be64toh(val); }
inline uint16_t hton(uint16_t val) { return htobe16(val); }
inline uint32_t hton(uint32_t val) { return htobe32(val); }
inline uint64_t hton(uint64_t val) { return htobe64(val); }

template<typename T1, typename T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    if(value % alignment != 0) {
        return value + alignment - (value % alignment);
    } else {
        return value;
    }
}

#endif // __UTILS_HPP__
