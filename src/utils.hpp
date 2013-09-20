// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef UTILS_HPP_
#define UTILS_HPP_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif  // VALGRIND

#include <stdexcept>
#include <string>
#include <vector>

#include "containers/printf_buffer.hpp"
#include "errors.hpp"
#include "config/args.hpp"

class Term;
void pb_print(Term *t);

// A thread number as used by the thread pool.
class threadnum_t {
public:
    explicit threadnum_t(int32_t _threadnum) : threadnum(_threadnum) { }

    bool operator==(threadnum_t other) const { return threadnum == other.threadnum; }

    int32_t threadnum;
};

class startup_shutdown_t {
public:
    startup_shutdown_t();
    ~startup_shutdown_t();
};

struct const_charslice {
    const char *beg, *end;
    const_charslice(const char *_beg, const char *_end) : beg(_beg), end(_end) { }
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

void *malloc_aligned(size_t size, size_t alignment);

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

timespec clock_monotonic();
timespec clock_realtime();

typedef uint64_t ticks_t;
ticks_t secs_to_ticks(time_t secs);
ticks_t get_ticks();
time_t get_secs();
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
void debug_print_quoted_string(printf_buffer_t *buf, const uint8_t *s, size_t n);
void debugf_prefix_buf(printf_buffer_t *buf);
void debugf_dump_buf(printf_buffer_t *buf);

// Primitive debug_print declarations.
void debug_print(printf_buffer_t *buf, uint64_t x);
void debug_print(printf_buffer_t *buf, const std::string& s);

#ifndef NDEBUG
void debugf(const char *msg, ...) __attribute__((format (printf, 1, 2)));
template <class T>
void debugf_print(const char *msg, const T &obj) {
    printf_buffer_t buf;
    debugf_prefix_buf(&buf);
    buf.appendf("%s: ", msg);
    debug_print(&buf, obj);
    buf.appendf("\n");
    debugf_dump_buf(&buf);
}
#else
#define debugf(...) ((void)0)
#define debugf_print(...) ((void)0)
#endif  // NDEBUG

template <class T>
std::string debug_strprint(const T &obj) {
    printf_buffer_t buf;
    debug_print(&buf, obj);
    return std::string(buf.data(), buf.size());
}

class debugf_in_dtor_t {
public:
    explicit debugf_in_dtor_t(const char *msg, ...) __attribute__((format (printf, 2, 3)));
    ~debugf_in_dtor_t();
private:
    std::string message;
};

class rng_t {
public:
// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
    int randint(int n);
    double randdouble();
    explicit rng_t(int seed = -1);
private:
    unsigned short xsubi[3];  // NOLINT(runtime/int)
    DISABLE_COPYING(rng_t);
};

// Reads from /dev/urandom.  Use this sparingly, please.
void get_dev_urandom(void *out, int64_t nbytes);

int randint(int n);
double randdouble();
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

std::string strprintf(const char *format, ...) __attribute__((format (printf, 1, 2)));
std::string vstrprintf(const char *format, va_list ap) __attribute__((format (printf, 1, 0)));


// formatted time:
// yyyy-mm-ddThh:mm:ss.nnnnnnnnn   (29 characters)
const size_t formatted_time_length = 29;    // not including null

void format_time(struct timespec time, printf_buffer_t *buf);
std::string format_time(struct timespec time);

struct timespec parse_time(const std::string &str) THROWS_ONLY(std::runtime_error);

/* Printing binary data to stderr in a nice format */
void print_hd(const void *buf, size_t offset, size_t length);

// Fast string compare

int sized_strcmp(const uint8_t *str1, int len1, const uint8_t *str2, int len2);


/* The home thread mixin is a mixin for objects that can only be used
on a single thread. Its thread ID is exposed as the `home_thread()`
method. Some subclasses of `home_thread_mixin_debug_only_t` can move themselves to
another thread, modifying the field real_home_thread. */

#define INVALID_THREAD (threadnum_t(-1))

class home_thread_mixin_debug_only_t {
public:
#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif

protected:
    explicit home_thread_mixin_debug_only_t(threadnum_t specified_home_thread);
    home_thread_mixin_debug_only_t();
    ~home_thread_mixin_debug_only_t() { }

#ifndef NDEBUG
    threadnum_t real_home_thread;
#endif
};

class home_thread_mixin_t {
public:
    threadnum_t home_thread() const { return real_home_thread; }
#ifndef NDEBUG
    void assert_thread() const;
#else
    void assert_thread() const { }
#endif

protected:
    explicit home_thread_mixin_t(threadnum_t specified_home_thread);
    home_thread_mixin_t();
    ~home_thread_mixin_t() { }

    threadnum_t real_home_thread;

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
    explicit on_thread_t(threadnum_t thread);
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

std::string blocking_read_file(const char *path);
bool blocking_read_file(const char *path, std::string *contents_out);

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
static inline std::string time2str(const time_t &t) {
    char timebuf[26]; // I apologize for the magic constant.
    //           ^^ See man 3 ctime_r
    return ctime_r(&t, timebuf);
}

std::string errno_string(int errsv);


int get_num_db_threads();

template <class T>
T valgrind_undefined(T value) {
#ifdef VALGRIND
    UNUSED auto x = VALGRIND_MAKE_MEM_UNDEFINED(&value, sizeof(value));
#endif
    return value;
}


// Contains the name of the directory in which all data is stored.
class base_path_t {
public:
    explicit base_path_t(const std::string& path);
    const std::string& path() const;

    // Make this base_path_t into an absolute path (useful for daemonizing)
    // This can only be done if the path already exists, which is why we don't do it at construction
    void make_absolute();
private:
    std::string path_;
};

static const char *TEMPORARY_DIRECTORY_NAME = "tmp";

class serializer_filepath_t;

namespace unittest {
serializer_filepath_t manual_serializer_filepath(const std::string& permanent_path,
                                                 const std::string& temporary_path);
}  // namespace unittest

// Contains the name of a serializer file.
class serializer_filepath_t {
public:
    serializer_filepath_t(const base_path_t& directory, const std::string& relative_path)
        : permanent_path_(directory.path() + "/" + relative_path),
          temporary_path_(directory.path() + "/" + TEMPORARY_DIRECTORY_NAME + "/" + relative_path + ".create") {
        guarantee(!relative_path.empty());
    }

    // A serializer_file_opener_t will first open the file in a temporary location, then move it to
    // the permanent location when it's finished being created.  These give the names of those
    // locations.
    std::string permanent_path() const { return permanent_path_; }
    std::string temporary_path() const { return temporary_path_; }

private:
    friend serializer_filepath_t unittest::manual_serializer_filepath(const std::string& permanent_path,
                                                                      const std::string& temporary_path);
    serializer_filepath_t(const std::string& permanent_path, const std::string& temporary_path)
        : permanent_path_(permanent_path), temporary_path_(temporary_path) { }

    const std::string permanent_path_;
    const std::string temporary_path_;
};

void recreate_temporary_directory(const base_path_t& base_path);

// This will be thrown by remove_directory_recursive if a file cannot be removed
class remove_directory_exc_t : public std::exception {
public:
    remove_directory_exc_t(const std::string &path, int err) :
        info(strprintf("Fatal error: failed to delete file '%s': %s.",
                       path.c_str(), strerror(err)))
    { }
    ~remove_directory_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    const std::string info;
};

void remove_directory_recursive(const char *path) THROWS_ONLY(remove_directory_exc_t);

bool ptr_in_byte_range(const void *p, const void *range_start, size_t size_in_bytes);
bool range_inside_of_byte_range(const void *p, size_t n_bytes, const void *range_start, size_t size_in_bytes);

class debug_timer_t {
public:
    debug_timer_t(std::string _name = "");
    ~debug_timer_t();
    microtime_t tick(const std::string &tag);
private:
    microtime_t start, last;
    std::string name, out;
};

#define MSTR(x) stringify(x) // Stringify a macro
#if defined __clang__
#define COMPILER "CLANG " __clang_version__
#elif defined __GNUC__
#define COMPILER "GCC " MSTR(__GNUC__) "." MSTR(__GNUC_MINOR__) "." MSTR(__GNUC_PATCHLEVEL__)
#else
#define COMPILER "UNKNOWN COMPILER"
#endif

#ifndef NDEBUG
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION " (debug)" " (" COMPILER ")"
#else
#define RETHINKDB_VERSION_STR "rethinkdb " RETHINKDB_VERSION " (" COMPILER ")"
#endif

#define NULLPTR (static_cast<void *>(0))

#define DBLPRI "%.20g"

#define ANY_PORT 0

#endif // UTILS_HPP_
