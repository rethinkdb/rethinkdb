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
#include "arch/core.hpp"
#include "utils2.hpp"
#include <string>

// Precise time (time+nanoseconds) for logging, etc.

struct precise_time_t : public tm {
    uint32_t ns;    // nanoseconds since the start of the second
                    // beware:
                    //   tm::tm_year is number of years since 1970,
                    //   tm::tm_mon is number of months since January,
                    //   tm::tm_sec is from 0 to 60 (to account for leap seconds)
                    // For more information see man gmtime(3)
};

void initialize_precise_time();     // should be called during startup
timespec get_uptime();              // returns relative time since initialize_precise_time(),
                                    // can return low precision time if clock_gettime call fails
precise_time_t get_absolute_time(const timespec& relative_time); // converts relative time to absolute
precise_time_t get_time_now();      // equivalent to get_absolute_time(get_uptime())

// formatted precise time:
// yyyy-mm-dd hh:mm:ss.MMMMMM   (26 characters)
const size_t formatted_precise_time_length = 26;    // not including null

void format_precise_time(const precise_time_t& time, char* buf, size_t max_chars);
std::string format_precise_time(const precise_time_t& time);

/* Printing binary data to stdout in a nice format */

void print_hd(const void *buf, size_t offset, size_t length);

// Fast string compare

int sized_strcmp(const char *str1, int len1, const char *str2, int len2);

std::string strip_spaces(std::string); 

/* `thread_saver_t` makes sure that its destructor returns on the same thread that its
constructor was called on. */

class thread_saver_t {
public:
    thread_saver_t() : thread_id_(get_thread_id()) {
        assert_good_thread_id(thread_id_);
    }
    explicit thread_saver_t(int thread_id) : thread_id_(thread_id) {
        assert_good_thread_id(thread_id_);
    }
    ~thread_saver_t() {
        coro_t::move_to_thread(thread_id_);
    }

private:
    int thread_id_;
    DISABLE_COPYING(thread_saver_t);
};

/* The home thread mixin is a mixin for objects that can only be used on
a single thread. Its thread ID is exposed as the `home_thread` variable. Some
subclasses of `home_thread_mixin_t` can be moved to another thread; to do this,
you can use the `rethread_t` type or the `rethread()` method. */

#define INVALID_THREAD (-1)

class home_thread_mixin_t {
public:
    const int &home_thread;
    int get_home_thread() const { return home_thread; }

    void assert_thread() const {
        if (home_thread == INVALID_THREAD) {
            crash("This object cannot be used because it currently does not have a home thread.\n");
        } else {
            rassert(home_thread == get_thread_id());
        }
    }

    // We force callers to pass a thread_saver_t to ensure that they
    // know exactly what they're doing.
    void ensure_thread(UNUSED const thread_saver_t& saver) const {
        if (home_thread == INVALID_THREAD) {
            crash("This object cannot be used because it currently does not have a home thread.\n");
        }
        // TODO: make sure nobody is calling move_to_thread except us
        // and thread_saver_t.
        coro_t::move_to_thread(home_thread);
    }

    virtual void rethread(UNUSED int thread) {
        crash("This class is not rethreadable.\n");
    }

    struct rethread_t {
        rethread_t(home_thread_mixin_t *m, int thread) :
                mixin(m), old_thread(mixin->home_thread), new_thread(thread) {
            mixin->rethread(new_thread);
            rassert(mixin->home_thread == new_thread);
        }
        ~rethread_t() {
            rassert(mixin->home_thread == new_thread);
            mixin->rethread(old_thread);
            rassert(mixin->home_thread == old_thread);
        }
    private:
        home_thread_mixin_t *mixin;
        int old_thread, new_thread;
    };

protected:
    home_thread_mixin_t() :
        home_thread(real_home_thread), real_home_thread(get_thread_id()) { }
    ~home_thread_mixin_t() { }

    // `home_thread` is a read-only version of `real_home_thread`.
    int real_home_thread;

private:
    // Things with home threads should not be copyable, since we don't
    // want to nonchalantly copy their home_thread variable.
    DISABLE_COPYING(home_thread_mixin_t);
};

/* `on_thread_t` switches to the given thread in its constructor, then switches
back in its destructor. For example:

    printf("Suppose we are on thread 1.\n");
    {
        on_thread_t thread_switcher(2);
        printf("Now we are on thread 2.\n");
    }
    printf("And now we are on thread 1 again.\n");

*/

struct on_thread_t : public home_thread_mixin_t {
    on_thread_t(int thread) {
        coro_t::move_to_thread(thread);
    }
    ~on_thread_t() {
        coro_t::move_to_thread(home_thread);
    }
};

/* `is_sorted()` returns true if the given range is sorted. */

template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator first, ForwardIterator last,
                       StrictWeakOrdering comp) {
    for(ForwardIterator it = first; it + 1 < last; it++) {
        if (!comp(*it, *(it+1)))
            return false;
    }
    return true;
}

/* API to allow a nicer way of performing jobs on other cores than subclassing
from thread_message_t. Call do_on_thread() with an object and a method for that object.
The method will be called on the other thread. If the thread to call the method on is
the current thread, returns the method's return value. Otherwise, returns false. */

template<class callable_t>
void do_on_thread(int thread, const callable_t& callable);

template<class callable_t>
void do_later(const callable_t &callable);

// Provides a compare operator which compares the dereferenced values of pointers T* (for use in std:sort etc)

template <typename obj_t>
class dereferencing_compare_t {
public:
    bool operator()(obj_t * const &o1, obj_t * const &o2) const {
        return *o1 < *o2;
    }
};

/* `make_vector()` provides a more concise way of constructing vectors with
only a few members. */

template<class value_t>
std::vector<value_t> make_vector(value_t v1) {
    std::vector<value_t> vec;
    vec.push_back(v1);
    return vec;
}

template<class value_t>
std::vector<value_t> make_vector(value_t v1, value_t v2) {
    std::vector<value_t> vec;
    vec.push_back(v1);
    vec.push_back(v2);
    return vec;
}

template<class value_t>
std::vector<value_t> make_vector(value_t v1, value_t v2, value_t v3) {
    std::vector<value_t> vec;
    vec.push_back(v1);
    vec.push_back(v2);
    vec.push_back(v3);
    return vec;
}

#include "utils.tcc"

#endif // __UTILS_HPP__
