
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
#include "errors.hpp"
#include "arch/arch.hpp"
#include "utils2.hpp"

// Precise time (time+nanoseconds) for logging, etc.
typedef struct {
    uint32_t nanosec;
    tm low_res;     // beware:
                    //   tm_year is number of years since 1970,
                    //   tm_mon is number of months since January,
                    //   tm_sec is from 0 to 60 (to account for leap seconds)
                    // For more information see man gmtime(3)
} precise_time_t;

void initialize_precise_time();     // should be called during startup
precise_time_t get_precise_time();
// format:
// yyyy-mm-dd hh:mm:ss.MMMMMM   (26 characters)
void format_precise_time(const precise_time_t& time, char* buf, size_t max_chars);
const size_t formatted_precise_time_length = 26;    // not including null

// The signal handler for SIGSEGV
void generic_crash_handler(int signum);
void ignore_crash_handler(int signum);
void install_generic_crash_handler();

void print_hd(const void *buf, size_t offset, size_t length);

// Fast string compare
int sized_strcmp(const char *str1, int len1, const char *str2, int len2);

// Buffer
template <int _size>
struct buffer_base_t
{
    char buf[_size];
    static const int size = _size;
};

template <int _size>
struct buffer_t : public buffer_base_t<_size>
{
};

/* The home thread mixin is a simple mixin for objects that are primarily associated with
a single thread. It just keeps track of the thread that the object was created on and
makes sure that it is destroyed from the same thread. It exposes the ID of that thread
as the "home_thread" variable. */

struct home_thread_mixin_t {
    int home_thread;
    home_thread_mixin_t() : home_thread(get_thread_id()) { }
    ~home_thread_mixin_t() { assert_thread(); }
    
#ifndef NDEBUG
    void assert_thread() { assert(home_thread == get_thread_id()); }
#else
    void assert_thread() { }
#endif
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

/* API to allow a nicer way of performing jobs on other cores than subclassing
from thread_message_t. Call do_on_thread() with an object and a method for that object.
The method will be called on the other thread. If the thread to call the method on is
the current thread, returns the method's return value. Otherwise, returns false. */

template<class callable_t>
bool do_on_thread(int thread, const callable_t &callable);

template<class obj_t>
bool do_on_thread(int thread, obj_t *obj, bool (obj_t::*on_other_core)());

template<class obj_t, class arg1_t>
bool do_on_thread(int thread, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t), arg1_t arg);

template<class obj_t, class arg1_t, class arg2_t>
bool do_on_thread(int thread, obj_t *obj, bool (obj_t::*on_other_core)(arg1_t, arg2_t), arg1_t arg1, arg2_t arg2);

template<class callable_t>
void do_later(const callable_t &callable);

template<class obj_t>
void do_later(obj_t *obj, bool (obj_t::*later)());

template<class obj_t, class arg1_t>
void do_later(obj_t *obj, bool (obj_t::*later)(arg1_t), arg1_t arg1);

#include "utils.tcc"

#endif // __UTILS_HPP__
