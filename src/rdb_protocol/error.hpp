// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ERROR_HPP_
#define RDB_PROTOCOL_ERROR_HPP_

#include <list>
#include <string>

#include "errors.hpp"

#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rpc/serialize_macros.hpp"

namespace ql {

class backtrace_id_t {
public:
    // The 0 ID corresponds to an empty backtrace
    backtrace_id_t() : id(0) { }
    explicit backtrace_id_t(uint32_t _id) : id(_id) { }
    static backtrace_id_t empty() {
        return backtrace_id_t();
    }
    uint32_t get() const { return id; }
private:
    uint32_t id;
    RDB_DECLARE_ME_SERIALIZABLE(backtrace_id_t);
};

// Catch this if you want to handle either `exc_t` or `datum_exc_t`.
class base_exc_t : public std::exception {
public:
    enum type_t {
        LOGIC, // An error in ReQL logic.
        INTERNAL, // An internal error.
        RESOURCE, // Exceeded a resource limit (e.g. the array size limit).
        RESUMABLE_OP_FAILED, // Used internally to retry some operations.
        OP_FAILED, // An operation is known to have failed.
        OP_INDETERMINATE, // It is unknown whether an operation failed or not.
        USER, // An error caused by `r.error` with arguments.
        EMPTY_USER, // An error caused by `r.error` with no arguments.
        NON_EXISTENCE, // An error related to the absence of an expected value.
        PERMISSION_ERROR // An error related to the user permissions.
    };
    explicit base_exc_t(type_t _type) : type(_type) { }
    virtual ~base_exc_t() throw () { }
    type_t get_type() const { return type; }
    virtual void rethrow_with_type(type_t type) const = 0;
    Response::ErrorType get_error_type() const {
        switch (type) {
        case EMPTY_USER:          // fallthru (this only bubbles up here if misused)
        case LOGIC:               return Response::QUERY_LOGIC;
        case INTERNAL:            return Response::INTERNAL;
        case RESOURCE:            return Response::RESOURCE_LIMIT;
        case RESUMABLE_OP_FAILED: // fallthru
        case OP_FAILED:           return Response::OP_FAILED;
        case OP_INDETERMINATE:    return Response::OP_INDETERMINATE;
        case USER:                return Response::USER;
        case NON_EXISTENCE:       return Response::NON_EXISTENCE;
        case PERMISSION_ERROR:    return Response::PERMISSION_ERROR;
        default: unreachable();
        }
        unreachable();
    }

protected:
    type_t type;
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    base_exc_t::type_t, int8_t, base_exc_t::LOGIC, base_exc_t::PERMISSION_ERROR);

// NOTE: you usually want to inherit from `rcheckable_t` instead of calling this
// directly.
NORETURN void runtime_fail(base_exc_t::type_t type,
                           const char *test, const char *file, int line,
                           std::string msg, backtrace_id_t bt);
NORETURN void runtime_fail(base_exc_t::type_t type,
                           const char *test, const char *file, int line,
                           std::string msg);
NORETURN void runtime_sanity_check_failed(
                   const char *file, int line, const char *test, const std::string &msg);

// Inherit from this in classes that wish to use `rcheck`.  If a class is
// rcheckable, it means that you can call `rcheck` from within it or use it as a
// target for `rcheck_target`.
class rcheckable_t {
public:
    virtual ~rcheckable_t() { }
    virtual void runtime_fail(base_exc_t::type_t type,
                              const char *test, const char *file, int line,
                              std::string msg) const = 0;
};

// This is a particular type of rcheckable.  A `bt_rcheckable_t` corresponds to
// a part of the protobuf source tree, and can be used to produce a useful
// backtrace.  (By contrast, a normal rcheckable might produce an error with no
// backtrace, e.g. if we some constraint that doesn't involve the user's code
// is violated.)
class bt_rcheckable_t : public rcheckable_t {
public:
    virtual void runtime_fail(base_exc_t::type_t type,
                              const char *test, const char *file, int line,
                              std::string msg) const {
        ql::runtime_fail(type, test, file, line, msg, bt);
    }

    backtrace_id_t backtrace() const { return bt; }

protected:
    explicit bt_rcheckable_t(backtrace_id_t _bt)
        : bt(_bt) { }

    backtrace_id_t update_bt(backtrace_id_t new_bt) {
        std::swap(new_bt, bt);
        return new_bt;
    }

private:
    backtrace_id_t bt;
};

// Helper function for formatting array over size error message.
std::string format_array_size_error(size_t limit);

// Use these macros to return errors to users.
// TODO: all these arguments should be in parentheses inside the expansion.
#define rcheck_target(target, pred, type, msg) do {                  \
        (pred)                                                       \
        ? (void)0                                                    \
        : (target)->runtime_fail(type, stringify(pred),              \
                                 __FILE__, __LINE__, (msg));         \
    } while (0)
#define rcheck_typed_target(target, pred, msg) do {                     \
        (pred)                                                          \
        ? (void)0                                                       \
        : (target)->runtime_fail(exc_type(target), stringify(pred),     \
                                 __FILE__, __LINE__, (msg));            \
    } while (0)
#define rcheck_src(src, pred, type, msg) do {                         \
        (pred)                                                        \
        ? (void)0                                                     \
        : ql::runtime_fail(type, stringify(pred),                     \
                           __FILE__, __LINE__, (msg), (src));         \
    } while (0)
#define rcheck_datum(pred, type, msg) do {                            \
        (pred)                                                        \
        ? (void)0                                                     \
        : ql::runtime_fail(type, stringify(pred),                     \
                           __FILE__, __LINE__, (msg));                \
    } while (0)
#define rcheck_array_size_datum(arr, limit) do {                        \
        auto _limit = (limit);                                          \
        rcheck_datum((arr).size() <= _limit.array_size_limit(),         \
                     ql::base_exc_t::RESOURCE,                          \
                     format_array_size_error(_limit     \
                              .array_size_limit()).c_str());     \
    } while (0)
#define rcheck_array_size(arr, limit) do {                              \
        auto _limit = (limit);                                          \
        rcheck((arr).size() <= _limit.array_size_limit(), ql::base_exc_t::RESOURCE, \
                    format_array_size_error(_limit     \
                        .array_size_limit()).c_str());     \
    } while (0)
#define rcheck(pred, type, msg) rcheck_target(this, pred, type, msg)
#define rcheck_toplevel(pred, type, msg) \
    rcheck_src(ql::backtrace_id_t::empty(), pred, type, msg)

#define rfail_datum(type, ...) do {                              \
        rcheck_datum(false, type, strprintf(__VA_ARGS__));       \
        unreachable();                                           \
    } while (0)
#define rfail_target(target, type, ...) do {                            \
        rcheck_target(target, false, type, strprintf(__VA_ARGS__));     \
        unreachable();                                                  \
    } while (0)
#define rfail_typed_target(target, ...) do {                            \
        rcheck_typed_target(target, false, strprintf(__VA_ARGS__));     \
        unreachable();                                                  \
    } while (0)
#define rfail_src(src, type, ...) do {                                  \
        rcheck_src(src, false, type, strprintf(__VA_ARGS__));           \
        unreachable();                                                  \
    } while (0)
#define rfail(type, ...) do {                                           \
        rcheck(false, type, strprintf(__VA_ARGS__));                    \
        unreachable();                                                  \
    } while (0)
#define rfail_toplevel(type, ...) do {                          \
        rcheck_toplevel(false, type, strprintf(__VA_ARGS__));   \
        unreachable();                                          \
    } while (0)
#define r_sanity_fail() do {               \
        r_sanity_check(false);             \
        unreachable();                     \
    } while (0)


class datum_t;
class val_t;
base_exc_t::type_t exc_type(const datum_t *d);
base_exc_t::type_t exc_type(const datum_t &d);
base_exc_t::type_t exc_type(const val_t *d);
base_exc_t::type_t exc_type(const scoped_ptr_t<val_t> &v);

// r_sanity_check should be used in place of guarantee if you think the
// guarantee will almost always fail due to an error in the query logic rather
// than memory corruption.
#ifndef NDEBUG
#define r_sanity_check(test, ...) guarantee(test, ##__VA_ARGS__)
#else
#define r_sanity_check(test, ...) do {                         \
        if (!(test)) {                                         \
            ::ql::runtime_sanity_check_failed(                 \
                __FILE__, __LINE__, stringify(test),           \
                strprintf(" " __VA_ARGS__).substr(1));         \
        }                                                      \
    } while (0)
#endif // NDEBUG

// A RQL exception.
class exc_t : public base_exc_t {
public:
    // We have a default constructor because these are serialized.
    exc_t() : base_exc_t(base_exc_t::LOGIC), message("UNINITIALIZED") { }
    exc_t(base_exc_t::type_t _type, const std::string &_message,
          backtrace_id_t _bt, size_t _dummy_frames = 0)
        : base_exc_t(_type), message(_message), bt(_bt), dummy_frames_(_dummy_frames) { }
    exc_t(const base_exc_t &e, backtrace_id_t _bt, size_t _dummy_frames = 0)
        : base_exc_t(e.get_type()), message(e.what()),
          bt(_bt), dummy_frames_(_dummy_frames) { }
    virtual ~exc_t() throw () { }

    const char *what() const throw () { return message.c_str(); }
    void rethrow_with_type(base_exc_t::type_t _type) const final {
        throw exc_t(_type, message, bt, dummy_frames_);
    }

    backtrace_id_t backtrace() const { return bt; }
    size_t dummy_frames() const { return dummy_frames_; }

    RDB_DECLARE_ME_SERIALIZABLE(exc_t);
private:
    std::string message;
    backtrace_id_t bt;
    size_t dummy_frames_;
};

// A datum exception is like a normal RQL exception, except it doesn't
// correspond to part of the source tree.  It's usually thrown from inside
// datum.{hpp,cc} and must be caught by the enclosing term/stream/whatever and
// turned into a normal `exc_t`.
class datum_exc_t : public base_exc_t {
public:
    datum_exc_t() : base_exc_t(base_exc_t::LOGIC), message("UNINITIALIZED") { }
    explicit datum_exc_t(base_exc_t::type_t _type, const std::string &_message)
        : base_exc_t(_type), message(_message) { }
    virtual ~datum_exc_t() throw () { }

    const char *what() const throw () { return message.c_str(); }
    void rethrow_with_type(base_exc_t::type_t _type) const final {
        throw datum_exc_t(_type, message);
    }
    RDB_DECLARE_ME_SERIALIZABLE(datum_exc_t);
private:
    std::string message;
};

} // namespace ql

#endif  // RDB_PROTOCOL_ERROR_HPP_
