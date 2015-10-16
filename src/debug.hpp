// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef DEBUG_HPP_
#define DEBUG_HPP_

#include <string>

#include "boost_utils.hpp"
#include "containers/printf_buffer.hpp"
#include "stl_utils.hpp"
#include "time.hpp"

#ifndef NDEBUG
#define trace_call(fn, ...) do {                                          \
        debugf("%s:%u: %s: entered\n", __FILE__, __LINE__, stringify(fn));  \
        fn(__VA_ARGS__);                                                           \
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
template <class T>
typename std::enable_if<std::is_arithmetic<T>::value>::type
debug_print(printf_buffer_t *buf, T x) {
    debug_print(buf, std::to_string(x));
}

void debug_print(printf_buffer_t *buf, const std::string& s);

template <class T>
void debug_print(printf_buffer_t *buf, T *ptr) {
    buf->appendf("%p", ptr);
}

template<class T>
std::string debug_str(const T &t) {
    printf_buffer_t buf;
    debug_print(&buf, t);
    return buf.c_str();
}

#ifndef NDEBUG
void debugf(const char *msg, ...) ATTR_FORMAT(printf, 1, 2);
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
    explicit debugf_in_dtor_t(const char *msg, ...) ATTR_FORMAT(printf, 2, 3);
    ~debugf_in_dtor_t();
private:
    std::string message;
};

// TODO: make this more efficient (use `clock_monotonic` and use a vector of
// integers rather than accumulating a string).
class debug_timer_t {
public:
    explicit debug_timer_t(std::string _name = "");
    ~debug_timer_t();
    microtime_t tick(const std::string &tag);
private:
    microtime_t start, last;
    std::string name, out;
    DISABLE_COPYING(debug_timer_t);
};

#endif  // DEBUG_HPP_

