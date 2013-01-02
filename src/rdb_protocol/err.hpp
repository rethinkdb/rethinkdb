#include <list>
#include <string>

#include "utils.hpp"

#include "rdb_protocol/ql2.pb.h"

#ifndef RDB_PROTOCOL_ERR_HPP_
#define RDB_PROTOCOL_ERR_HPP_
namespace ql {

void _runtime_check(const char *test, const char *file, int line,
                    bool pred, std::string msg = "");
#define rcheck(pred, msg) \
    _runtime_check(stringify(pred), __FILE__, __LINE__, pred, msg)
// TODO: do something smarter?
#define rfail(args...) rcheck(false, strprintf(args))
// TODO: make this crash in debug mode
#define r_sanity_check(test) rcheck(test, "SANITY_CHECK")

struct backtrace_t {
    struct frame_t {
    public:
        frame_t(int _pos) : type(POS), pos(_pos) { }
        frame_t(const std::string &_opt) : type(OPT), opt(_opt) { }
        Response2_Frame toproto() const;
    private:
        enum type_t { POS = 0, OPT = 1 };
        type_t type;
        int pos;
        std::string opt;
    };
    std::list<frame_t> frames;
};

class exc_t : public std::exception {
public:
    exc_t(const std::string &_msg) : msg(_msg) { }
    virtual ~exc_t() throw () { }
    backtrace_t backtrace;
    const char *what() const throw () { return msg.c_str(); }
private:
    const std::string msg;
};

void fill_error(Response2 *res, Response2_ResponseType type, std::string msg,
                const backtrace_t &bt=backtrace_t());

}
#endif // RDB_PROTOCOL_ERR_HPP_
