#ifndef CONTAINERS_ARCHIVE_ARCHIVE_HPP_
#define CONTAINERS_ARCHIVE_ARCHIVE_HPP_

#include <stdint.h>

#include "errors.hpp"
#include "containers/intrusive_list.hpp"

class uuid_t;

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
    ARCHIVE_GENERIC_ERROR = 1
};

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
    virtual int64_t write(const void *p, int64_t n) = 0;
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

    intrusive_list_t<write_buffer_t> *unsafe_expose_buffers() { return &buffers_; }

    // This _could_ destroy the object yo.
    template <class T>
    write_message_t &operator<<(const T &x) {
        x.rdb_serialize(*this);
        return *this;
    }

private:
    friend int send_write_message(write_stream_t *s, const write_message_t *msg);

    intrusive_list_t<write_buffer_t> buffers_;

    DISABLE_COPYING(write_message_t);
};

MUST_USE int send_write_message(write_stream_t *s, const write_message_t *msg);

#define ARCHIVE_PRIM_MAKE_WRITE_SERIALIZABLE(typ1, typ2)                \
    inline write_message_t &operator<<(write_message_t &msg, typ1 x) {  \
        union {                                                         \
            typ2 v;                                                     \
            char buf[sizeof(typ2)];                                     \
        } u;                                                            \
        u.v = x;                                                        \
        msg.append(u.buf, sizeof(typ2));                                \
        return msg;                                                     \
    }                                                                   \
    void archive_prim_make_write_serializable_nonsensical_declaration(typ1, typ2)

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
    }                                                                   \
    void archive_prim_make_ranged_serializable_nonsensical_declaration(typ1, typ2)

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
            return ARCHIVE_SOCK_ERROR;                                  \
        }                                                               \
        if (res < int64_t(sizeof(typ))) {                               \
            return ARCHIVE_SOCK_EOF;                                    \
        }                                                               \
        *x = u.v;                                                       \
        return ARCHIVE_SUCCESS;                                         \
    }                                                                   \
    void archive_prim_make_raw_serializable_nonsensical_declaration(typ)

ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(int8_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(uint8_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(int16_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(uint16_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(int32_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(uint32_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(int64_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(uint64_t);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(float);
ARCHIVE_PRIM_MAKE_RAW_SERIALIZABLE(double);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(bool, int8_t, 0, 1);

write_message_t &operator<<(write_message_t &msg, const uuid_t &uuid);
MUST_USE archive_result_t deserialize(read_stream_t *s, uuid_t *uuid);


#endif  // CONTAINERS_ARCHIVE_ARCHIVE_HPP_
