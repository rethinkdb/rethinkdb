// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ERROR_HPP_
#define RDB_PROTOCOL_ERROR_HPP_

#include <list>
#include <string>

#include "errors.hpp"

#include "containers/archive/archive.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/ql2_extensions.pb.h"
#include "rpc/serialize_macros.hpp"

namespace ql {

// Catch this if you want to handle either `exc_t` or `datum_exc_t`.
class base_exc_t : public std::exception {
public:
    enum type_t {
        GENERIC, // All errors except those below.

        // The only thing that cares about these is `default`.
        EMPTY_USER, // An error caused by `r.error` with no arguments.
        NON_EXISTENCE // An error related to the absence of an expected value.
    };
    explicit base_exc_t(type_t type) : type_(type) { }
    virtual ~base_exc_t() throw () { }
    type_t get_type() const { return type_; }
protected:
    type_t type_;
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    base_exc_t::type_t, int8_t, base_exc_t::GENERIC, base_exc_t::NON_EXISTENCE);

// NOTE: you usually want to inherit from `rcheckable_t` instead of calling this
// directly.
void runtime_fail(base_exc_t::type_t type,
                  const char *test, const char *file, int line,
                  std::string msg, const Backtrace *bt_src) NORETURN;
void runtime_fail(base_exc_t::type_t type,
                  const char *test, const char *file, int line,
                  std::string msg) NORETURN;
void runtime_sanity_check_failed(
    const char *file, int line, const char *test, const std::string &msg) NORETURN;

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

protob_t<const Backtrace> get_backtrace(const protob_t<const Term> &t);

// This is a particular type of rcheckable.  A `pb_rcheckable_t` corresponds to
// a part of the protobuf source tree, and can be used to produce a useful
// backtrace.  (By contrast, a normal rcheckable might produce an error with no
// backtrace, e.g. if we some constraint that doesn't involve the user's code
// is violated.)
class pb_rcheckable_t : public rcheckable_t {
public:
    virtual void runtime_fail(base_exc_t::type_t type,
                              const char *test, const char *file, int line,
                              std::string msg) const {
        ql::runtime_fail(type, test, file, line, msg, bt_src.get());
    }

    // Propagate the associated backtrace through the rewrite term.
    void propagate(Term *t) const;

    const protob_t<const Backtrace> &backtrace() const { return bt_src; }

protected:
    explicit pb_rcheckable_t(const protob_t<const Backtrace> &_bt_src)
        : bt_src(_bt_src) { }

    protob_t<const Backtrace> update_bt(const protob_t<const Backtrace> &new_bt) {
        protob_t<const Backtrace> ret(new_bt);
        ret.swap(bt_src);
        return std::move(ret);
    }

private:
    protob_t<const Backtrace> bt_src;
};

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
#define rcheck_array_size_datum(arr, limit, type) do {                  \
        auto _limit = (limit);                                          \
        rcheck_datum((arr).size() <= _limit.array_size_limit(), (type),  \
                     strprintf("Array over size limit `%zu`.",          \
                               _limit.array_size_limit()).c_str());     \
    } while (0)
#define rcheck_array_size(arr, limit, type) do {                        \
        auto _limit = (limit);                                          \
        rcheck((arr).size() <= _limit.array_size_limit(), type,          \
               strprintf("Array over size limit `%zu`.",                \
                         _limit.array_size_limit()).c_str());           \
    } while (0)
#define rcheck(pred, type, msg) rcheck_target(this, pred, type, msg)
#define rcheck_toplevel(pred, type, msg) \
    rcheck_src(NULL, pred, type, msg)

#define rfail_datum(type, args...) do {                          \
        rcheck_datum(false, type, strprintf(args));              \
        unreachable();                                           \
    } while (0)
#define rfail_target(target, type, args...) do {                 \
        rcheck_target(target, false, type, strprintf(args));     \
        unreachable();                                           \
    } while (0)
#define rfail_typed_target(target, args...) do {                  \
        rcheck_typed_target(target, false, strprintf(args));      \
        unreachable();                                            \
    } while (0)
#define rfail_src(src, type, args...) do {                       \
        rcheck_src(src, false, type, strprintf(args));            \
        unreachable();                                           \
    } while (0)
#define rfail(type, args...) do {                                       \
        rcheck(false, type, strprintf(args));                           \
        unreachable();                                                  \
    } while (0)
#define rfail_toplevel(type, args...) do {               \
        rcheck_toplevel(false, type, strprintf(args));   \
        unreachable();                                   \
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
#define r_sanity_check(test, msg...) guarantee(test, ##msg)
#else
#define r_sanity_check(test, msg...) do {                      \
        if (!(test)) {                                         \
            ::ql::runtime_sanity_check_failed(                 \
                __FILE__, __LINE__, stringify(test),           \
                strprintf(" " msg).substr(1));                 \
        }                                                      \
    } while (0)
#endif // NDEBUG

// A backtrace we return to the user.  Pretty self-explanatory.
class backtrace_t {
public:
    explicit backtrace_t(const Backtrace *bt) {
        if (!bt) return;
        for (int i = 0; i < bt->frames_size(); ++i) {
            frame_t f(bt->frames(i));
            r_sanity_check(f.is_valid());
            frames.push_back(f);
        }
    }
    backtrace_t() { }

    class frame_t {
    public:
        frame_t() : type(OPT), opt("UNITIALIZED") { }
        explicit frame_t(int32_t _pos) : type(POS), pos(_pos) { }
        explicit frame_t(const std::string &_opt) : type(OPT), opt(_opt) { }
        explicit frame_t(const char *_opt) : type(OPT), opt(_opt) { }
        explicit frame_t(const Frame &f);
        Frame toproto() const;

        static frame_t invalid() { return frame_t(INVALID); }
        bool is_invalid() const { return type == POS && pos == INVALID; }
        static frame_t head() { return frame_t(HEAD); }
        bool is_head() const { return type == POS && pos == HEAD; }
        static frame_t skip() { return frame_t(SKIP); }
        bool is_skip() const { return type == POS && pos == SKIP; }
        bool is_valid() { // -1 is the classic "invalid" frame
            return is_head() || is_skip()
                || (type == POS && pos >= 0)
                || (type == OPT && opt != "UNINITIALIZED");
        }
        bool is_stream_funcall_frame() {
            return type == POS && pos != 0;
        }

        RDB_DECLARE_ME_SERIALIZABLE(frame_t);

    private:
        enum special_frames {
            INVALID = -1,
            HEAD = -2,
            SKIP = -3
        };
        enum type_t { POS = 0, OPT = 1 };
        int32_t type; // serialize macros didn't like `type_t` for some reason
        int32_t pos;
        std::string opt;
    };

    void fill_bt(Backtrace *bt) const;
    // Write out the backtrace to return it to the user.
    void fill_error(Response *res, Response_ResponseType type, std::string msg) const;

    bool is_empty() { return frames.size() == 0; }

    void delete_frames(int num_frames) {
        for (int i = 0; i < num_frames; ++i) {
            if (frames.size() == 0) {
                rassert(false);
            } else {
                frames.pop_back();
            }
        }
    }

    RDB_DECLARE_ME_SERIALIZABLE(backtrace_t);
private:
    std::list<frame_t> frames;
};

const backtrace_t::frame_t head_frame = backtrace_t::frame_t::head();

// A RQL exception.
class exc_t : public base_exc_t {
public:
    // We have a default constructor because these are serialized.
    exc_t() : base_exc_t(base_exc_t::GENERIC), exc_msg_("UNINITIALIZED") { }
    exc_t(base_exc_t::type_t type, const std::string &exc_msg,
          const Backtrace *bt_src)
        : base_exc_t(type), exc_msg_(exc_msg) {
        if (bt_src != NULL) {
            backtrace_ = backtrace_t(bt_src);
        }
    }
    exc_t(const base_exc_t &e, const Backtrace *bt_src, int dummy_frames = 0)
        : base_exc_t(e.get_type()), exc_msg_(e.what()) {
        if (bt_src != NULL) {
            backtrace_ = backtrace_t(bt_src);
        }
        backtrace_.delete_frames(dummy_frames);
    }
    exc_t(base_exc_t::type_t type, const std::string &exc_msg,
          const backtrace_t &backtrace)
        : base_exc_t(type), backtrace_(backtrace), exc_msg_(exc_msg) { }
    virtual ~exc_t() throw () { }

    const char *what() const throw () { return exc_msg_.c_str(); }
    const backtrace_t &backtrace() const { return backtrace_; }

    RDB_DECLARE_ME_SERIALIZABLE(exc_t);
private:
    backtrace_t backtrace_;
    std::string exc_msg_;
};

// A datum exception is like a normal RQL exception, except it doesn't
// correspond to part of the source tree.  It's usually thrown from inside
// datum.{hpp,cc} and must be caught by the enclosing term/stream/whatever and
// turned into a normal `exc_t`.
class datum_exc_t : public base_exc_t {
public:
    datum_exc_t() : base_exc_t(base_exc_t::GENERIC), exc_msg("UNINITIALIZED") { }
    explicit datum_exc_t(base_exc_t::type_t type, const std::string &_exc_msg)
        : base_exc_t(type), exc_msg(_exc_msg) { }
    virtual ~datum_exc_t() throw () { }
    const char *what() const throw () { return exc_msg.c_str(); }

    RDB_DECLARE_ME_SERIALIZABLE(datum_exc_t);
private:
    std::string exc_msg;
};

void fill_error(Response *res, Response_ResponseType type, std::string msg,
                const backtrace_t &bt = backtrace_t());

} // namespace ql

#endif  // RDB_PROTOCOL_ERROR_HPP_
