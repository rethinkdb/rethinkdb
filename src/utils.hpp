// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <stdio.h>
#include <stdlib.h>

#include <functional>
#include <stdexcept>
#include <string>

#include "errors.hpp"
#include "config/args.hpp"

class printf_buffer_t;

namespace ph = std::placeholders;

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

/* Forbid the following function definition to be inlined
 * (note: some compilers might require `noinline` instead of `__attribute__ ((noinline))`)
 */
#define NOINLINE __attribute__ ((noinline))

void *malloc_aligned(size_t size, size_t alignment);

/* Calls `malloc()` and checks its return value to crash if the allocation fails. */
void *rmalloc(size_t size);
/* Calls `realloc()` and checks its return value to crash if the allocation fails. */
void *rrealloc(void *ptr, size_t size);




class rng_t {
public:
// Returns a random number in [0, n).  Is not perfectly uniform; the
// bias tends to get worse when RAND_MAX is far from a multiple of n.
    int randint(int n);
    uint64_t randuint64(uint64_t n);
    double randdouble();
    explicit rng_t(int seed = -1);
private:
    unsigned short xsubi[3];  // NOLINT(runtime/int)
    DISABLE_COPYING(rng_t);
};

// Reads from /dev/urandom.  Use this sparingly, please.
void get_dev_urandom(void *out, int64_t nbytes);

int randint(int n);
uint64_t randuint64(uint64_t n);
size_t randsize(size_t n);
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



/* `with_priority_t` changes the priority of the current coroutine to the
 value given in its constructor. When it is destructed, it restores the
 original priority of the coroutine. */

class with_priority_t {
public:
    explicit with_priority_t(int priority);
    ~with_priority_t();
private:
    int previous_priority;
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
    remove_directory_exc_t(const std::string &path, int errsv) {
        char buf[512];
        info = strprintf("Fatal error: failed to delete file '%s': %s.",
                         path.c_str(),
                         errno_string_maybe_using_buffer(errsv, buf, sizeof(buf)));
    }
    ~remove_directory_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
private:
    std::string info;
};

void remove_directory_recursive(const char *path) THROWS_ONLY(remove_directory_exc_t);

bool ptr_in_byte_range(const void *p, const void *range_start, size_t size_in_bytes);
bool range_inside_of_byte_range(const void *p, size_t n_bytes, const void *range_start, size_t size_in_bytes);

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

#define ANY_PORT 0

#endif // UTILS_HPP_
