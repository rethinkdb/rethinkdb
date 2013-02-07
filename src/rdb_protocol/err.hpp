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
#define rcheck(pred, msg) \
    _runtime_check(stringify(pred), __FILE__, __LINE__, pred, msg)
// TODO: do something smarter?
#define rfail(args...) rcheck(false, strprintf(args))
#ifndef NDEBUG
#define r_sanity_check(test) guarantee(test)
#else
#define r_sanity_check(test) rcheck(test, "SANITY CHECK FAILED (server is buggy)")
#endif // NDEBUG

struct backtrace_t {
    struct frame_t {
    public:
        frame_t() : type(OPT), opt("UNITIALIZED") { }
        frame_t(int _pos) : type(POS), pos(_pos) { }
        frame_t(const std::string &_opt) : type(OPT), opt(_opt) { }
        frame_t(const char *_opt) : type(OPT), opt(_opt) { }
        frame_t(const frame_t &rhs) : type(rhs.type), pos(rhs.pos), opt(rhs.opt) { }
        Response2_Frame toproto() const;

        static frame_t head() { return frame_t(-1337); }
        bool is_head() const { return type == POS && pos == -1337; }
        bool is_valid() {
            return is_head()
                || (type == POS && pos >= 0)
                || (type == OPT && opt != "UNINITIALIZED");
        }
    private:
        enum type_t { POS = 0, OPT = 1 };
        int type; // serialize macros didn't like `type_t` for some reason
        int pos;
        std::string opt;

    public:
        RDB_MAKE_ME_SERIALIZABLE_3(type, pos, opt);
    };
    void fill_error(Response2 *res, Response2_ResponseType type, std::string msg) const;
    RDB_MAKE_ME_SERIALIZABLE_1(frames);

    void push_front(frame_t f) {
        r_sanity_check(f.is_valid());
        // debugf("PUSHING %s\n", f.toproto().DebugString().c_str());
        frames.push_front(f);
    }
private:
    std::list<frame_t> frames;
};

class exc_t : public std::exception {
public:
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

}

#define WITH_BT(N, cmd)                         \
    try {                                       \
        cmd;                                    \
    } catch (exc_t &e) {                        \
        e.backtrace.push_front(N);              \
        throw;                                  \
    }

#define CATCH_WITH_BT(N) catch (exc_t &e) { \
        e.backtrace.push_front(N);          \
        throw;                              \
    }

#endif // RDB_PROTOCOL_ERR_HPP_
