#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdio.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <functional>
#include <string>
#include <vector>

#include "errors.hpp"
#include "arch/core.hpp"
#include "arch/coroutines.hpp"
#include "utils2.hpp"

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


/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_t` can be moved to
another thread; to do this, you can use the `rethread_t` type or the
`rethread()` method. */

#define INVALID_THREAD (-1)

class home_thread_mixin_t {
public:
    int home_thread() const { return real_home_thread; }

    void assert_thread() const {
        rassert(home_thread() == get_thread_id());
    }

    virtual void rethread(int thread);

    struct rethread_t {
        rethread_t(home_thread_mixin_t *m, int thread);
        ~rethread_t();
    private:
        home_thread_mixin_t *mixin;
        int old_thread, new_thread;
    };

protected:
    home_thread_mixin_t();
    virtual ~home_thread_mixin_t() { }

    int real_home_thread;

private:
    // Things with home threads should not be copyable, since we don't
    // want to nonchalantly copy their real_home_thread variable.
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
    on_thread_t(int thread);
    ~on_thread_t();
};


/* API to allow a nicer way of performing jobs on other cores than subclassing
from thread_message_t. Call do_on_thread() with an object and a method for that object.
The method will be called on the other thread. If the thread to call the method on is
the current thread, returns the method's return value. Otherwise, returns false. */

template<class callable_t>
void do_on_thread(int thread, const callable_t& callable);

#include "utils.tcc"

#endif // __UTILS_HPP__
