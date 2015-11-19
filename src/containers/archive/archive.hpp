// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_ARCHIVE_HPP_
#define CONTAINERS_ARCHIVE_ARCHIVE_HPP_

#include <stdint.h>

#include <string>
#include <type_traits>

#include "containers/printf_buffer.hpp"
#include "containers/intrusive_list.hpp"
#include "version.hpp"
#include "valgrind.hpp"

class uuid_u;

struct fake_archive_exc_t {
    const char *what() const throw() {
        return "Writing to a tcp stream failed.";
    }
};

class read_stream_t {
public:
    read_stream_t() { }
    // Returns number of bytes read or 0 upon EOF, -1 upon error.
    virtual MUST_USE int64_t read(void *p, int64_t n) = 0;
protected:
    virtual ~read_stream_t() { }
private:
    DISABLE_COPYING(read_stream_t);
};

// The return value of deserialization functions.
enum class archive_result_t {
    // Success.
    SUCCESS,
    // An error on the socket happened.
    SOCK_ERROR,
    // An EOF on the socket happened.
    SOCK_EOF,
    // The value deserialized was out of range.
    RANGE_ERROR,
};

inline bool bad(archive_result_t res) {
    return res != archive_result_t::SUCCESS;
}

const char *archive_result_as_str(archive_result_t archive_result);

#define guarantee_deserialization(result, ...) do {                     \
        guarantee(result == archive_result_t::SUCCESS,                  \
                  "Deserialization of %s failed with error %s.",        \
                  strprintf(__VA_ARGS__).c_str(),                       \
                  archive_result_as_str(result));                       \
    } while (0)

class archive_exc_t : public std::exception {
public:
    explicit archive_exc_t(std::string _s) : s(std::move(_s)) { }
    ~archive_exc_t() throw () { }
    const char *what() const throw() {
        return s.c_str();
    }
private:
    std::string s;
};

#define throw_if_bad_deserialization(result, ...) do {                  \
        if (result != archive_result_t::SUCCESS) {                      \
            throw archive_exc_t(                                        \
                strprintf("Deserialization of %s failed with error %s.", \
                          strprintf(__VA_ARGS__).c_str(),               \
                          archive_result_as_str(result)));              \
        }                                                               \
    } while (0)

// Returns the number of bytes written, or -1.  Returns a
// non-negative value less than n upon EOF.
MUST_USE int64_t force_read(read_stream_t *s, void *p, int64_t n);

class write_stream_t {
public:
    write_stream_t() { }
    // Returns n, or -1 upon error. Blocks until all bytes are written.
    virtual MUST_USE int64_t write(const void *p, int64_t n) = 0;
protected:
    virtual ~write_stream_t() { }
private:
    DISABLE_COPYING(write_stream_t);
};

class write_buffer_t : public intrusive_list_node_t<write_buffer_t> {
public:
    write_buffer_t() : size(0) { }

    static const int DATA_SIZE = 4096;
    int size;
    char data[DATA_SIZE];

private:
    DISABLE_COPYING(write_buffer_t);
};

// A set of buffers in which an atomic message to be sent on a stream
// gets built up.  (This way we don't flush after the first four bytes
// sent to a stream, or buffer things and then forget to manually
// flush.  This type can be extended to support holding references to
// large buffers, to save copying.)  Generally speaking, you serialize
// to a write_message_t, and then flush that to a write_stream_t.
class write_message_t {
public:
    write_message_t() { }
    explicit write_message_t(write_message_t &&) = default;
    ~write_message_t();

    void append(const void *p, int64_t n);

    size_t size() const;

    intrusive_list_t<write_buffer_t> *unsafe_expose_buffers() { return &buffers_; }

private:
    friend int send_write_message(write_stream_t *s, const write_message_t *wm);

    intrusive_list_t<write_buffer_t> buffers_;

    DISABLE_COPYING(write_message_t);
};

// Returns 0 upon success, -1 upon failure.
MUST_USE int send_write_message(write_stream_t *s, const write_message_t *wm);

template <class T>
T *deserialize_deref(T &val) {  // NOLINT(runtime/references)
    return &val;
}

template <class T>
struct serialized_size_t;
template <class T>
struct serialize_universal_size_t;

// Makes typ1 serializable, sending a typ2 over the wire.  Has range
// checking on the closed interval [lo, hi] when deserializing.
#define ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(typ1, typ2, lo, hi)       \
    template <cluster_version_t W>                                      \
    void serialize(write_message_t *wm, typ1 x) {                       \
        union {                                                         \
            typ2 v;                                                     \
            char buf[sizeof(typ2)];                                     \
        } u;                                                            \
        u.v = static_cast<typ2>(x);                                     \
        rassert(u.v >= typ2(lo) && u.v <= typ2(hi));                    \
        wm->append(u.buf, sizeof(typ2));                                \
    }                                                                   \
                                                                        \
    template <cluster_version_t W>                                      \
    inline MUST_USE archive_result_t deserialize(read_stream_t *s, typ1 *x) { \
        union {                                                         \
            typ2 v;                                                     \
            char buf[sizeof(typ2)];                                     \
        } u;                                                            \
        int64_t res = force_read(s, u.buf, sizeof(typ2));               \
        if (res == -1) {                                                \
            return archive_result_t::SOCK_ERROR;                        \
        }                                                               \
        if (res < int64_t(sizeof(typ2))) {                              \
            return archive_result_t::SOCK_EOF;                          \
        }                                                               \
        if (u.v < typ2(lo) || u.v > typ2(hi)) {                         \
            return archive_result_t::RANGE_ERROR;                       \
        }                                                               \
        *x = typ1(u.v);                                                 \
        return archive_result_t::SUCCESS;                               \
    }

// Designed for <stdint.h>'s u?int[0-9]+_t types, which are just sent
// raw over the wire.
//
// serialize_universal and deserialize_universal are functions whose behavior must
// never change: if you want to serialize values differently, make a different
// function.
#define ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(typ)                         \
    inline void serialize_universal(write_message_t *wm, typ x) {       \
        union {                                                         \
            typ v;                                                      \
            char buf[sizeof(typ)];                                      \
        } u;                                                            \
        u.v = x;                                                        \
        wm->append(u.buf, sizeof(typ));                                 \
    }                                                                   \
    template <cluster_version_t W>                                      \
    void serialize(write_message_t *wm, typ x) {                        \
        serialize_universal(wm, x);                                     \
    }                                                                   \
                                                                        \
    inline MUST_USE archive_result_t                                    \
    deserialize_universal(read_stream_t *s, typ *x) {                   \
        union {                                                         \
            typ v;                                                      \
            char buf[sizeof(typ)];                                      \
        } u;                                                            \
        int64_t res = force_read(s, u.buf, sizeof(typ));                \
        if (res == -1) {                                                \
            *x = valgrind_undefined<typ>(0);                            \
            return archive_result_t::SOCK_ERROR;                        \
        }                                                               \
        if (res < int64_t(sizeof(typ))) {                               \
            *x = valgrind_undefined<typ>(0);                            \
            return archive_result_t::SOCK_EOF;                          \
        }                                                               \
        *x = u.v;                                                       \
        return archive_result_t::SUCCESS;                               \
    }                                                                   \
                                                                        \
    template <cluster_version_t W>                                      \
    MUST_USE archive_result_t deserialize(read_stream_t *s, typ *x) {   \
        return deserialize_universal(s, x);                             \
    }                                                                   \
                                                                        \
    template <>                                                         \
    struct serialized_size_t<typ>                                       \
        : public std::integral_constant<size_t, sizeof(typ)> { }; /* NOLINT(readability/braces) */       \
    template <>                                                         \
    struct serialize_universal_size_t<typ>                              \
        : public std::integral_constant<size_t, sizeof(typ)> { }



ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(unsigned char);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(char);          // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(signed char);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(unsigned short);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(signed short);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(unsigned int);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(signed int);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(unsigned long);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(signed long);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(unsigned long long);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(signed long long);  // NOLINT(runtime/int)
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(double);

// Single-precision float is omitted on purpose, don't add it.  Go
// change your code to use doubles.

// Note: serialize_universal depends on this not changing.
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(bool, int8_t, 0, 1);
template <>
struct serialized_size_t<bool> : public serialized_size_t<int8_t> { };
template <>
struct serialize_universal_size_t<bool> : public serialize_universal_size_t<int8_t> { };

void serialize_universal(write_message_t *wm, bool b);
MUST_USE archive_result_t deserialize_universal(read_stream_t *s, bool *b);

void serialize_universal(write_message_t *wm, const uuid_u &uuid);
MUST_USE archive_result_t deserialize_universal(read_stream_t *s, uuid_u *uuid);

template <cluster_version_t W>
void serialize(write_message_t *wm, const uuid_u &uuid);
template <cluster_version_t W>
MUST_USE archive_result_t deserialize(read_stream_t *s, uuid_u *uuid);

struct in_addr;
struct in6_addr;

template <cluster_version_t W>
void serialize(write_message_t *wm, const in_addr &addr);
template <cluster_version_t W>
MUST_USE archive_result_t deserialize(read_stream_t *s, in_addr *addr);

template <cluster_version_t W>
void serialize(write_message_t *wm, const in6_addr &addr);
template <cluster_version_t W>
MUST_USE archive_result_t deserialize(read_stream_t *s, in6_addr *addr);

#ifdef _MSC_VER
#define RDB_IMPL_DESERIALIZE_TEMPLATE(T, ...)                           \
    template <cluster_version_t W>                                      \
    MUST_USE archive_result_t deserialize(read_stream_t *s, T<__VA_ARGS__> *x) { \
        deserialize<W, __VA_ARGS__>(s, x);                              \
    }
#else
#define RDB_IMPL_DESERIALIZE_TEMPLATE
#endif

#endif  // CONTAINERS_ARCHIVE_ARCHIVE_HPP_
