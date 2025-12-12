// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file archive.hpp
/// @brief Serialization framework for RPC and persistence
///
/// Provides the foundation for serializing and deserializing RethinkDB data types
/// over network streams and to disk. Handles version-aware serialization with
/// support for forward/backward compatibility.
///
/// @defgroup Serialization Serialization and Deserialization Framework
/// Framework for converting objects to/from binary format
/// @{

#ifndef CONTAINERS_ARCHIVE_ARCHIVE_HPP_
#define CONTAINERS_ARCHIVE_ARCHIVE_HPP_

#include <stdint.h>

#include <string>
#include <type_traits>

#include "containers/intrusive_list.hpp"
#include "containers/printf_buffer.hpp"
#include "version.hpp"
#include "valgrind.hpp"

class uuid_u;

/// @brief Exception thrown when writing to an archive fails
struct fake_archive_exc_t {
    /// @brief Gets the error message
    const char *what() const throw() {
        return "Writing to a tcp stream failed.";
    }
};

/// @brief Abstract base class for reading from an archive stream
///
/// `read_stream_t` defines the interface for reading data from various sources
/// (network sockets, files, memory buffers). Implementations provide the actual
/// I/O mechanics.
///
/// @example
/// @code
/// // Custom read stream implementation
/// class custom_read_stream : public read_stream_t {
/// public:
///     int64_t read(void *p, int64_t n) override {
///         // Read n bytes from source into p
///         // Return: bytes read, 0 for EOF, -1 for error
///     }
/// };
/// @endcode
class read_stream_t {
public:
    /// @brief Default constructor
    read_stream_t() { }

    /// @brief Reads data from the stream
    /// Reads up to n bytes from the stream into the provided buffer.
    /// @param p Pointer to buffer where data is written
    /// @param n Maximum number of bytes to read
    /// @return Number of bytes read (1-n), 0 on EOF, -1 on error
    /// @note Implementations should read exactly n bytes when possible
    virtual MUST_USE int64_t read(void *p, int64_t n) = 0;

protected:
    /// @brief Virtual destructor for proper cleanup
    virtual ~read_stream_t() { }

private:
    DISABLE_COPYING(read_stream_t);
};

/// @brief Result status of a deserialization operation
///
/// Indicates the outcome of attempting to deserialize data from a stream.
/// Used instead of exceptions in some code paths for error handling.
enum class archive_result_t {
    SUCCESS,        ///< Deserialization succeeded
    SOCK_ERROR,     ///< Socket/stream error occurred
    SOCK_EOF,       ///< End-of-file reached unexpectedly
    RANGE_ERROR,    ///< Deserialized value out of valid range
};

/// @brief Checks if an archive result indicates an error
/// @param res The archive result to check
/// @return true if the result is not SUCCESS
inline bool bad(archive_result_t res) {
    return res != archive_result_t::SUCCESS;
}

/// @brief Converts an archive_result_t to a human-readable string
/// @param archive_result The result code to convert
/// @return String description of the result
const char *archive_result_as_str(archive_result_t archive_result);

/// @brief Macro that guarantees successful deserialization
/// Aborts the program if the result indicates an error.
/// @param result The archive_result_t to check
/// @param ... Format string and arguments for the guarantee message
/// @example
/// @code
/// archive_result_t result = deserialize_universal(stream, &value);
/// guarantee_deserialization(result, "my_struct");
/// @endcode
#define guarantee_deserialization(result, ...) do {                     \
        guarantee(result == archive_result_t::SUCCESS,                  \
                  "Deserialization of %s failed with error %s.",        \
                  strprintf(__VA_ARGS__).c_str(),                       \
                  archive_result_as_str(result));                       \
    } while (0)

/// @brief Exception thrown for serialization/deserialization errors
///
/// Used by the archive framework to report errors in a C++ exception context.
/// Carries a descriptive error message.
class archive_exc_t : public std::exception {
public:
    /// @brief Constructs an archive exception with a message
    /// @param _s The error message
    explicit archive_exc_t(std::string _s) : s(std::move(_s)) { }

    /// @brief Virtual destructor
    ~archive_exc_t() throw () { }

    /// @brief Gets the error message
    /// @return C-string describing the error
    const char *what() const throw() {
        return s.c_str();
    }

private:
    std::string s;  ///< The error message
};

/// @brief Macro that throws an exception if deserialization fails
/// Throws archive_exc_t with a formatted message if the result is not SUCCESS.
/// @param result The archive_result_t to check
/// @param ... Format string and arguments for the exception message
/// @example
/// @code
/// archive_result_t result = deserialize_universal(stream, &value);
/// throw_if_bad_deserialization(result, "my_struct");
/// @endcode
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

#endif  // CONTAINERS_ARCHIVE_ARCHIVE_HPP_
