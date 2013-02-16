#ifndef RDB_PROTOCOL_ERR_HPP_
#define RDB_PROTOCOL_ERR_HPP_

#include <list>
#include <string>

#include "utils.hpp"

#include "containers/archive/stl_types.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rpc/serialize_macros.hpp"

namespace ql {

void _runtime_check(const char *test, const char *file, int line,
                    bool pred, std::string msg = "");

// Use these macros to return errors to users.
#define rcheck(pred, msg)                                               \
    _runtime_check(stringify(pred), __FILE__, __LINE__, pred, msg)
#define rfail(args...) rcheck(false, strprintf(args))


// r_sanity_check should be used in place of guarantee if you think the
// guarantee will almost always fail due to an error in the query logic rather
// than memory corruption.
#ifndef NDEBUG
#define r_sanity_check(test) guarantee(test)
#else
#define r_sanity_check(test) rcheck(test, "SANITY CHECK FAILED (server is buggy)")
#endif // NDEBUG

// A backtrace we return to the user.  Pretty self-explanatory.
struct backtrace_t {
    struct frame_t {
    public:
        explicit frame_t() : type(OPT), opt("UNITIALIZED") { }
        explicit frame_t(int _pos) : type(POS), pos(_pos) { }
        explicit frame_t(const std::string &_opt) : type(OPT), opt(_opt) { }
        explicit frame_t(const char *_opt) : type(OPT), opt(_opt) { }
        Response2_Frame toproto() const;

        static frame_t head() { return frame_t(HEAD); }
        bool is_head() const { return type == POS && pos == HEAD; }
        static frame_t skip() { return frame_t(SKIP); }
        bool is_skip() const { return type == POS && pos == SKIP; }
        bool is_valid() { // -1 is the classic "invalid" frame
            return is_head() || is_skip()
                || (type == POS && pos >= 0)
                || (type == OPT && opt != "UNINITIALIZED");
        }
    private:
        enum special_frames {
            INVALID = -1,
            HEAD = -2,
            SKIP = -3
        };
        enum type_t { POS = 0, OPT = 1 };
        int type; // serialize macros didn't like `type_t` for some reason
        int pos;
        std::string opt;

    public:
        RDB_MAKE_ME_SERIALIZABLE_3(type, pos, opt);
    };
    // Write out the backtrace to return it to the user.
    void fill_error(Response2 *res, Response2_ResponseType type, std::string msg) const;
    RDB_MAKE_ME_SERIALIZABLE_1(frames);

    // Push a frame onto the front of the backtrace.
    void push_front(frame_t f) {
        r_sanity_check(f.is_valid());
        // debugf("PUSHING %s\n", f.toproto().DebugString().c_str());
        frames.push_front(f);
    }
    template<class T>
    void push_front(T t) {
        push_front(frame_t(t));
    }
private:
    std::list<frame_t> frames;
};

const backtrace_t::frame_t head_frame = backtrace_t::frame_t::head();
const backtrace_t::frame_t skip_frame = backtrace_t::frame_t::skip();

// A RQL exception.  In the future it will be tagged.
class exc_t : public std::exception {
public:
    // We have a default constructor because these are serialized.
    exc_t() : exc_msg("UNINITIALIZED") { }
    exc_t(const std::string &_exc_msg) : exc_msg(_exc_msg) { }
    virtual ~exc_t() throw () { }
    backtrace_t backtrace;
    const char *what() const throw () { return exc_msg.c_str(); }
private:
    std::string exc_msg;

public:
    RDB_MAKE_ME_SERIALIZABLE_2(backtrace, exc_msg);
};

void fill_error(Response2 *res, Response2_ResponseType type, std::string msg,
                const backtrace_t &bt=backtrace_t());

} // namespace ql

#define CATCH_WITH_BT(N) catch (exc_t &e) { \
        e.backtrace.push_front(N);          \
        throw;                              \
    }

#endif // RDB_PROTOCOL_ERR_HPP_
