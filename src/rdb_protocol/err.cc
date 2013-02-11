#include "rdb_protocol/err.hpp"

namespace ql {
void _runtime_check(const char *test, const char *file, int line,
                    bool pred, std::string msg) {
    if (pred) return;
#ifndef NDEBUG
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw exc_t(msg);
}

void backtrace_t::fill_error(Response2 *res, Response2_ResponseType type,
                             std::string msg) const {
    guarantee(type == Response2::CLIENT_ERROR ||
              type == Response2::COMPILE_ERROR ||
              type == Response2::RUNTIME_ERROR);
    Datum error_msg;
    error_msg.set_type(Datum::R_STR);
    error_msg.set_r_str(msg);
    res->set_type(type);
    *res->add_response() = error_msg;
    for (std::list<backtrace_t::frame_t>::const_iterator
             it = frames.begin(); it != frames.end(); ++it) {
        if (it->is_head()) {
            rassert(it == frames.begin());
            continue;
        }
        *res->add_backtrace() = it->toproto();
    }
}
void fill_error(Response2 *res, Response2_ResponseType type, std::string msg,
                const backtrace_t &bt) {
    bt.fill_error(res, type, msg);
}

Response2_Frame backtrace_t::frame_t::toproto() const {
    Response2_Frame f;
    switch(type) {
    case POS: {
        f.set_type(Response2_Frame_FrameType_POS);
        f.set_pos(pos);
    }; break;
    case OPT: {
        f.set_type(Response2_Frame_FrameType_OPT);
        f.set_opt(opt);
    }; break;
    default: unreachable();
    }
    return f;
}

} // namespace ql
