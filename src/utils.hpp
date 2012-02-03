#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/function.hpp>

#include <list>
#include <map>

/* Note that repli_timestamp_t does NOT represent an actual timestamp; instead it's an arbitrary
counter. */

// for safety
class repli_timestamp_t {
public:
    uint32_t time;

    bool operator==(repli_timestamp_t t) const { return time == t.time; }
    bool operator!=(repli_timestamp_t t) const { return time != t.time; }
    bool operator<(repli_timestamp_t t) const { return time < t.time; }
    bool operator>(repli_timestamp_t t) const { return time > t.time; }
    bool operator<=(repli_timestamp_t t) const { return time <= t.time; }
    bool operator>=(repli_timestamp_t t) const { return time >= t.time; }

    repli_timestamp_t next() const {
        repli_timestamp_t t;
        t.time = time + 1;
        return t;
    }

    static const repli_timestamp_t distant_past;
    static const repli_timestamp_t invalid;

    /* Make `repli_timestamp_t` serializable using `boost::serialization` */
    template<class Archive>
    void serialize(Archive & ar, UNUSED unsigned int version) {
        ar & time;
    }
};

inline std::ostream &operator<<(std::ostream &os, repli_timestamp_t ts) {
    os << ts.time;
    return os;
}

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *beg_, const char *end_) : beg(beg_), end(end_) { }
    const_charslice() : beg(NULL), end(NULL) { }
};

typedef uint64_t microtime_t;

microtime_t current_microtime();

/* General exception to be thrown when some process is interrupted. It's in
`utils.hpp` because I can't think where else to put it */
class interrupted_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "interrupted";
    }
};

/* `death_runner_t` runs an arbitrary function in its destructor */
class death_runner_t {
public:
    death_runner_t() { }
    explicit death_runner_t(const boost::function<void()> &f) : fun(f) { }
    ~death_runner_t() {
        if (!fun.empty()) fun();
    }
    boost::function<void()> fun;
};

/* `map_insertion_sentry_t` inserts a value into a map on construction, and
removes it in the destructor. */
template<class key_t, class value_t>
class map_insertion_sentry_t {
public:
    map_insertion_sentry_t() : map(NULL) { }
    map_insertion_sentry_t(std::map<key_t, value_t> *m, const key_t &key, const value_t &value) : map(NULL) {
        reset(m, key, value);
    }
    ~map_insertion_sentry_t() {
        reset();
    }
    void reset() {
        if (map) {
            map->erase(it);
            map = NULL;
        }
    }
    void reset(std::map<key_t, value_t> *m, const key_t &key, const value_t &value) {
        reset();
        map = m;
        std::pair<typename std::map<key_t, value_t>::iterator, bool> iterator_and_is_new =
            map->insert(std::make_pair(key, value));
        rassert(iterator_and_is_new.second, "value to be inserted already "
            "exists. don't do that.");
        it = iterator_and_is_new.first;
    }
private:
    std::map<key_t, value_t> *map;
    typename std::map<key_t, value_t>::iterator it;
};

/* `multimap_insertion_sentry_t` inserts a value into a multimap on
construction, and removes it in the destructor. */
template<class key_t, class value_t>
class multimap_insertion_sentry_t {
public:
    multimap_insertion_sentry_t() : map(NULL) { }
    multimap_insertion_sentry_t(std::multimap<key_t, value_t> *m, const key_t &key, const value_t &value) : map(NULL) {
        reset(m, key, value);
    }
    ~multimap_insertion_sentry_t() {
        reset();
    }
    void reset() {
        if (map) {
            map->erase(it);
            map = NULL;
        }
    }
    void reset(std::multimap<key_t, value_t> *m, const key_t &key, const value_t &value) {
        reset();
        map = m;
        it = map->insert(std::make_pair(key, value));
    }
private:
    std::multimap<key_t, value_t> *map;
    typename std::multimap<key_t, value_t>::iterator it;
};

/* Binary blob that represents some unknown POD type */
class binary_blob_t {
public:
    binary_blob_t() { }

    template<class obj_t>
    explicit binary_blob_t(const obj_t &o) : storage(reinterpret_cast<const uint8_t *>(&o), reinterpret_cast<const uint8_t *>(&o + 1)) { }

    binary_blob_t(const uint8_t *data, size_t size) : storage(data, data+size) { }
    template<class InputIterator>
    binary_blob_t(InputIterator begin, InputIterator end) : storage(begin, end) { }

    /* Constructor in static method form so we can use it as a functor */
    template<class obj_t>
    static binary_blob_t make(const obj_t &o) {
        return binary_blob_t(o);
    }

    size_t size() const {
        return storage.size();
    }

    // Unfortunately C++ compiler gets confused by two overloaded functions
    // which differ only in constness of the argument, so they have to be named
    // differently.
    template<class obj_t>
    static obj_t &get_modifiable(binary_blob_t &blob) {
        rassert(blob.size() == sizeof(obj_t));
        return *reinterpret_cast<obj_t *>(blob.data());
    }

    template<class obj_t>
    static const obj_t &get(const binary_blob_t &blob) {
        rassert(blob.size() == sizeof(obj_t));
        return *reinterpret_cast<const obj_t *>(blob.data());
    }

    void *data() {
        return storage.data();
    }

    const void *data() const {
        return storage.data();
    }

private:
    std::vector<uint8_t> storage;
};

// Like std::max, except it's technically not associative.
repli_timestamp_t repli_max(repli_timestamp_t x, repli_timestamp_t y);


void *malloc_aligned(size_t size, size_t alignment = 64);

template <class T1, class T2>
T1 ceil_aligned(T1 value, T2 alignment) {
    return value + alignment - (((value + alignment - 1) % alignment) + 1);
}

template <class T1, class T2>
T1 ceil_divide(T1 dividend, T2 alignment) {
    return (dividend + alignment - 1) / alignment;
}

template <class T1, class T2>
T1 floor_aligned(T1 value, T2 alignment) {
    return value - (value % alignment);
}

template <class T1, class T2>
T1 ceil_modulo(T1 value, T2 alignment) {
    T1 x = (value + alignment - 1) % alignment;
    return value + alignment - ((x < 0 ? x + alignment : x) + 1);
}

inline bool divides(int64_t x, int64_t y) {
    return y % x == 0;
}

int gcd(int x, int y);

typedef unsigned long long ticks_t;
ticks_t secs_to_ticks(float secs);
ticks_t get_ticks();
time_t get_secs();
long get_ticks_res();
double ticks_to_secs(ticks_t ticks);

// HEY: Maybe debugf and log_call and TRACEPOINT should be placed in
// debug.hpp (and debug.cc).
/* Debugging printing API (prints current thread in addition to message) */
#ifndef NDEBUG
void debugf(const char *msg, ...) __attribute__((format (printf, 1, 2)));
#else
#define debugf(...) ((void)0)
#endif

class rng_t {
public:
// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
    int randint(int n);
    rng_t();
private:
    struct drand48_data buffer_;
    DISABLE_COPYING(rng_t);
};


std::string rand_string(int len);

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins with a minus
// sign. strtoul_strict() and strtoull_strict() do.
long strtol_strict(const char *string, char **end, int base);
unsigned long strtoul_strict(const char *string, char **end, int base);
unsigned long long strtoull_strict(const char *string, char **end, int base);

// This is inefficient, it calls vsnprintf twice and copies the
// arglist and output buffer excessively.
std::string strprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

/* `demangle_cpp_name()` attempts to de-mangle the given symbol name. If it
succeeds, it returns the result as a `std::string`. If it fails, it throws
`demangle_failed_exc_t`. */
struct demangle_failed_exc_t : public std::exception {
    const char *what() const throw () {
        return "Could not demangle C++ name.";
    }
};
std::string demangle_cpp_name(const char *mangled_name);

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
void set_precise_time_offset(timespec hi_res_clock, time_t low_res_clock);  // used in unit-tests
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


/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_t` can move themselves to
another thread, modifying the field real_home_thread. */

#define INVALID_THREAD (-1)

class home_thread_mixin_t {
public:
    int home_thread() const { return real_home_thread; }

#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif  // NDEBUG

protected:
    explicit home_thread_mixin_t(int specified_home_thread);
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

class on_thread_t : public home_thread_mixin_t {
public:
    explicit on_thread_t(int thread);
    ~on_thread_t();
};

/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
boost::uuids::uuid generate_uuid();

void print_backtrace(FILE *out = stderr, bool use_addr2line = true);

/* I think we basically all know this... but this function has linear
 * complexity and thus you can't use it for anything real, if you want to do
 * this type of access pattern use a different STL container. This only exists
 * because it's convenient to pass around paths as std::lists and I don't want
 * to write 3 lines of code to access the second element. */
template <class T>
T const &nth(std::list<T> const &l, unsigned n) {
    typename std::list<T>::const_iterator it = l.begin();

    while (n > 0) {
        rassert(it != l.end(), "n > list.size()");
        n--;
        it++;
    }

    return *it;
}

//TODO change this things name
template <class T, class K>
bool std_contains(const T &target, K const & key) {
    return target.find(key) != target.end();
}
template <class T, class K>
bool std_does_not_contain(const T &target, K const & key) {
    return !std_contains(target, key);
}

template <class InputIterator, class UnaryPredicate>
bool all_match_predicate(InputIterator begin, InputIterator end, UnaryPredicate f) {
    bool res = true;
    for (; begin != end; begin++) {
        res &= f(*begin);
    }
    return res;
}

template <class T, class UnaryPredicate>
bool all_in_container_match_predicate (const T &container, UnaryPredicate f) {
    return all_match_predicate(container.begin(), container.end(), f);
}

bool notf(bool x);

#endif // __UTILS_HPP__
