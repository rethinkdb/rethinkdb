#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "containers/printf_buffer.hpp"
#include "errors.hpp"
#include "config/args.hpp"


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

/* Pad a value to the size of a cache line to avoid false sharing.
 * TODO: This is implemented as a struct with subtraction rather than a union
 * so that it gives an error when trying to pad a value bigger than
 * CACHE_LINE_SIZE. If that's needed, this may have to be done differently.
 */
template<typename value_t>
struct cache_line_padded_t {
    value_t value;
    char padding[CACHE_LINE_SIZE - sizeof(value_t)];
};

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

int64_t round_up_to_power_of_two(int64_t x);

typedef uint64_t ticks_t;
ticks_t secs_to_ticks(float secs);
ticks_t get_ticks();
time_t get_secs();
int64_t get_ticks_res();
double ticks_to_secs(ticks_t ticks);

#ifndef NDEBUG
#define trace_call(fn, args...) do {                                          \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(args);                                                           \
        debugf("%s:%u: %s: returned\n", __FILE__, __LINE__, stringify(fn)); \
    } while (0)
#define TRACEPOINT debugf("%s:%u reached\n", __FILE__, __LINE__)
#else
#define trace_call(fn, args...) fn(args)
// TRACEPOINT is not defined in release, so that TRACEPOINTS do not linger in the code unnecessarily
#endif

// HEY: Maybe debugf and log_call and TRACEPOINT should be placed in
// debugf.hpp (and debugf.cc).
/* Debugging printing API (prints current thread in addition to message) */
void debug_print_quoted_string(append_only_printf_buffer_t *buf, const uint8_t *s, size_t n);
void debugf_prefix_buf(printf_buffer_t<1000> *buf);
void debugf_dump_buf(printf_buffer_t<1000> *buf);

// Primitive debug_print declarations.
void debug_print(append_only_printf_buffer_t *buf, uint64_t x);
void debug_print(append_only_printf_buffer_t *buf, const std::string& s);

#ifndef NDEBUG
void debugf(const char *msg, ...) __attribute__((format (printf, 1, 2)));
template <class T>
void debugf_print(const char *msg, const T& obj) {
    printf_buffer_t<1000> buf;
    debugf_prefix_buf(&buf);
    buf.appendf("%s: ", msg);
    debug_print(&buf, obj);
    buf.appendf("\n");
    debugf_dump_buf(&buf);
}
#else
#define debugf(...) ((void)0)
#define debugf_print(...) ((void)0)
#endif

class debugf_in_dtor_t {
public:
    explicit debugf_in_dtor_t(const char *msg, ...);
    ~debugf_in_dtor_t();
private:
    std::string message;
};

class rng_t {
public:
// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
    int randint(int n);
    explicit rng_t(int seed = -1);
private:
#ifndef __MACH__
    struct drand48_data buffer_;
#endif
    DISABLE_COPYING(rng_t);
};

int randint(int n);
std::string rand_string(int len);

bool begins_with_minus(const char *string);
// strtoul() and strtoull() will for some reason not fail if the input begins
// with a minus sign. strtou64_strict() does.  Also we fix the constness of the
// end parameter.
int64_t strtoi64_strict(const char *string, const char **end, int base);
uint64_t strtou64_strict(const char *string, const char **end, int base);

// These functions return false and set the result to 0 if the conversion fails or
// does not consume the whole string.
MUST_USE bool strtoi64_strict(const std::string &str, int base, int64_t *out_result);
MUST_USE bool strtou64_strict(const std::string &str, int base, uint64_t *out_result);

std::string strprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
std::string vstrprintf(const char *format, va_list ap);


// formatted time:
// yyyy-mm-ddThh:mm:ss.nnnnnnnnn   (29 characters)
const size_t formatted_time_length = 29;    // not including null

void format_time(struct timespec time, append_only_printf_buffer_t *buf);
std::string format_time(struct timespec time);

struct timespec parse_time(const std::string &str) THROWS_ONLY(std::runtime_error);

/* Printing binary data to stdout in a nice format */

void print_hd(const void *buf, size_t offset, size_t length);

// Fast string compare

int sized_strcmp(const uint8_t *str1, int len1, const uint8_t *str2, int len2);


/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_debug_only_t` can move themselves to
another thread, modifying the field real_home_thread. */

#define INVALID_THREAD (-1)

class home_thread_mixin_debug_only_t {
public:
    void assert_thread() const;

protected:
    explicit home_thread_mixin_debug_only_t(int specified_home_thread);
    home_thread_mixin_debug_only_t();
    virtual ~home_thread_mixin_debug_only_t() { }

private:
    DISABLE_COPYING(home_thread_mixin_debug_only_t);

#ifndef NDEBUG
    int real_home_thread;
#endif
};

class home_thread_mixin_t {
public:
    int home_thread() const { return real_home_thread; }
    void assert_thread() const;

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

/* Translates to and from `0123456789ABCDEF`. */
bool hex_to_int(char c, int *out);
char int_to_hex(int i);

std::string read_file(const char *path);

struct path_t {
    std::vector<std::string> nodes;
    bool is_absolute;
};

path_t parse_as_path(const std::string &);
std::string render_as_path(const path_t &);

enum region_join_result_t { REGION_JOIN_OK, REGION_JOIN_BAD_JOIN, REGION_JOIN_BAD_REGION };

template <class T>
class assignment_sentry_t {
public:
    assignment_sentry_t(T *v, const T &value) :
        var(v), old_value(*var) {
        *var = value;
    }
    ~assignment_sentry_t() {
        *var = old_value;
    }
private:
    T *var;
    T old_value;
};

std::string sanitize_for_logger(const std::string &s);

#endif // UTILS_HPP_
