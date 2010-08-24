
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <functional>
#include <vector>
#include <endian.h>
#include <arpa/inet.h>
#include "corefwd.hpp"
#include "config/code.hpp"   // For alloc_t

static inline void check(const char *msg, bool err) {
    if (err) {
        if(errno == 0)
            errno = EINVAL;
        perror(msg);
        abort();
    }
}

#define fail(msg) _fail(msg, __FILE__, __LINE__)
static inline void _fail(const char*, const char*, int) __attribute__ ((noreturn));
static inline void _fail(const char *msg, const char *file, int line) {
    fprintf(stderr, "%s:%d: %s\n", file, line, msg);
    abort();
}

int get_cpu_count();
long get_available_ram();
long get_total_ram();


void *malloc_aligned(size_t size, size_t alignment = 64);

// Replacements for new
template <typename T>
T* gnew();
template <typename T, typename A1>
T* gnew(A1);
template <typename T, typename A1, typename A2>
T* gnew(A1, A2);
template <typename T, typename A1, typename A2, typename A3>
T* gnew(A1, A2, A3);
template <typename T, typename A1, typename A2, typename A3, typename A4>
T* gnew(A1, A2, A3, A4);
template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
T* gnew(A1, A2, A3, A4, A5);
template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
T* gnew(A1, A2, A3, A4, A5, A6);

template<typename T>
void gdelete(T *p) {
    delete p;
}

// STL gnew allocator
template<typename T>
class gnew_alloc;

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

//endianness converter for host to rethinkdb endianness
/* uint16_t rtoh(uint16_t val) { return be16toh(val); }
uint32_t rtoh(uint32_t val) { return be32toh(val); }
uint64_t rtoh(uint64_t val) { return be64toh(val); }
uint16_t htor(uint16_t val) { return htobe16(val); }
uint32_t htor(uint32_t val) { return htobe32(val); }
uint64_t htor(uint64_t val) { return htobe64(val); } */

#include "utils.tcc"
#endif // __UTILS_HPP__
