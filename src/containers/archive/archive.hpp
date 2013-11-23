// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_ARCHIVE_HPP_
#define CONTAINERS_ARCHIVE_ARCHIVE_HPP_

#include <stdint.h>

#include "containers/intrusive_list.hpp"
#include "utils.hpp"

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

// Deserialize functions return 0 upon success, a positive or negative error
// code upon failure. -1 means there was an error on the socket, -2 means EOF on
// the socket, -3 means a "range error", +1 means specific error info was
// discarded, the error code got used as a boolean.
enum archive_result_t {
    ARCHIVE_SUCCESS = 0,
    ARCHIVE_SOCK_ERROR = -1,
    ARCHIVE_SOCK_EOF = -2,
    ARCHIVE_RANGE_ERROR = -3,
    ARCHIVE_GENERIC_ERROR = 1,
};

const char *archive_result_as_str(archive_result_t archive_result);

#define guarantee_deserialization(result, ...) do {                     \
        guarantee(result == ARCHIVE_SUCCESS,                            \
                  "Deserialization of %s failed with error %s.",        \
                  strprintf(__VA_ARGS__).c_str(),                       \
                  archive_result_as_str(result));                       \
    } while (0)

// We wrap things in this class for making friend declarations more
// compilable under gcc-4.5.
class archive_deserializer_t {
private:
    template <class T> friend archive_result_t deserialize(read_stream_t *s, T *thing);

    template <class T>
    static MUST_USE archive_result_t deserialize(read_stream_t *s, T *thing) {
        return thing->rdb_deserialize(s);
    }

    archive_deserializer_t();
};

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, T *thing) {
    return archive_deserializer_t::deserialize(s, thing);
}

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
    ~write_message_t();

    void append(const void *p, int64_t n);

    size_t size() const;

    intrusive_list_t<write_buffer_t> *unsafe_expose_buffers() { return &buffers_; }

    template <class T>
    void append_value(const T &x) {
        x.rdb_serialize(*this);
    }

private:
    friend int send_write_message(write_stream_t *s, const write_message_t *msg);

    intrusive_list_t<write_buffer_t> buffers_;

    DISABLE_COPYING(write_message_t);
};

template <class T>
write_message_t &operator<<(write_message_t& msg, const T &x) {
    msg.append_value(x);
    return msg;
}

// Returns 0 upon success, -1 upon failure.
MUST_USE int send_write_message(write_stream_t *s, const write_message_t *msg);

template <class T>
T *deserialize_deref(T &val) {
    return &val;
}

template <class T>
class empty_ok_t;

template <class T>
class empty_ok_ref_t {
public:
    T *get() const { return ptr_; }

private:
    template <class U>
    friend empty_ok_ref_t<U> deserialize_deref(const empty_ok_t<U> &val);

    explicit empty_ok_ref_t(T *ptr) : ptr_(ptr) { }

    T *ptr_;
};

// Used by empty_ok.
template <class T>
class empty_ok_t {
public:
    T *get() const { return ptr_; }

private:
    template <class U>
    friend empty_ok_t<U> empty_ok(U &field);

    explicit empty_ok_t(T *ptr) : ptr_(ptr) { }

    T *ptr_;
};

template <class T>
empty_ok_ref_t<T> deserialize_deref(const empty_ok_t<T> &val) {
    return empty_ok_ref_t<T>(val.get());
}

// A convenient wrapper for marking fields (of smart pointer types, typically) as being
// allowed to be serialized empty.  For example, counted_t<const datum_t> typically
// must be non-empty when serialized, but a special implementation for type
// empty_ok_t<counted_t<const datum_t> > is made that lets you serialize empty datum's.
// Simply wrap the name with empty_ok(...) in the serialization macro.
template <class T>
empty_ok_t<T> empty_ok(T &field) {
    return empty_ok_t<T>(&field);
}

template <class T>
struct serialized_size_t;



// Keep in sync with serialized_size_t defined below.
#define ARCHIVE_PRIM_MAKE_WRITE_SERIALIZABLE(typ1, typ2)                \
    inline write_message_t &operator<<(write_message_t &msg, typ1 x) {  \
        union {                                                         \
            typ2 v;                                                     \
            char buf[sizeof(typ2)];                                     \
        } u;                                                            \
        u.v = static_cast<typ2>(x);                                     \
        msg.append(u.buf, sizeof(typ2));                                \
        return msg;                                                     \
    }


// Makes typ1 serializable, sending a typ2 over the wire.  Has range
// checking on the closed interval [lo, hi] when deserializing.
#define ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(typ1, typ2, lo, hi)       \
    ARCHIVE_PRIM_MAKE_WRITE_SERIALIZABLE(typ1, typ2);                   \
                                                                        \
    inline MUST_USE archive_result_t deserialize(read_stream_t *s, typ1 *x) { \
        union {                                                         \
            typ2 v;                                                     \
            char buf[sizeof(typ2)];                                     \
        } u;                                                            \
        int64_t res = force_read(s, u.buf, sizeof(typ2));               \
        if (res == -1) {                                                \
            return ARCHIVE_SOCK_ERROR;                                  \
        }                                                               \
        if (res < int64_t(sizeof(typ2))) {                              \
            return ARCHIVE_SOCK_EOF;                                    \
        }                                                               \
        if (u.v < typ2(lo) || u.v > typ2(hi)) {                         \
            return ARCHIVE_RANGE_ERROR;                                 \
        }                                                               \
        *x = typ1(u.v);                                                 \
        return ARCHIVE_SUCCESS;                                         \
    }

// Designed for <stdint.h>'s u?int[0-9]+_t types, which are just sent
// raw over the wire.
#define ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(typ)                         \
    ARCHIVE_PRIM_MAKE_WRITE_SERIALIZABLE(typ, typ);                     \
                                                                        \
    inline MUST_USE archive_result_t deserialize(read_stream_t *s, typ *x) { \
        union {                                                         \
            typ v;                                                      \
            char buf[sizeof(typ)];                                      \
        } u;                                                            \
        int64_t res = force_read(s, u.buf, sizeof(typ));                \
        if (res == -1) {                                                \
            *x = valgrind_undefined<typ>(0);                            \
            return ARCHIVE_SOCK_ERROR;                                  \
        }                                                               \
        if (res < int64_t(sizeof(typ))) {                               \
            *x = valgrind_undefined<typ>(0);                            \
            return ARCHIVE_SOCK_EOF;                                    \
        }                                                               \
        *x = u.v;                                                       \
        return ARCHIVE_SUCCESS;                                         \
    }                                                                   \
                                                                        \
    template <>                                                         \
    struct serialized_size_t<typ>                                       \
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

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(bool, int8_t, 0, 1);
template <>
struct serialized_size_t<bool> : public serialized_size_t<int8_t> { };

write_message_t &operator<<(write_message_t &msg, const uuid_u &uuid);
MUST_USE archive_result_t deserialize(read_stream_t *s, uuid_u *uuid);

struct in_addr;
struct in6_addr;

write_message_t &operator<<(write_message_t &msg, const in_addr &addr);
MUST_USE archive_result_t deserialize(read_stream_t *s, in_addr *addr);

write_message_t &operator<<(write_message_t &msg, const in6_addr &addr);
MUST_USE archive_result_t deserialize(read_stream_t *s, in6_addr *addr);

#endif  // CONTAINERS_ARCHIVE_ARCHIVE_HPP_
