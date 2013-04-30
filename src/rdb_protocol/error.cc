#include "rdb_protocol/error.hpp"

#include "backtrace.hpp"
#include "rdb_protocol/term_walker.hpp"

namespace ql {
void runtime_check(DEBUG_VAR const char *test, DEBUG_VAR const char *file,
                   DEBUG_VAR int line, bool pred,
                   std::string msg, const Backtrace *bt_src,
                   int dummy_frames) {
    if (pred) return;
#ifndef NDEBUG
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw exc_t(msg, bt_src, dummy_frames);
}
void runtime_check(DEBUG_VAR const char *test, DEBUG_VAR const char *file,
                   DEBUG_VAR int line, bool pred, std::string msg) {
    if (pred) return;
#ifndef NDEBUG
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw datum_exc_t(msg);
}

void runtime_sanity_check(bool test) {
    if (!test) {
        lazy_backtrace_t bt;
        throw exc_t("SANITY CHECK FAILED (server is buggy).  Backtrace:\n" + bt.addrs(),
                    backtrace_t());
    }
}

void backtrace_t::fill_bt(Backtrace *bt) const {
    for (std::list<backtrace_t::frame_t>::const_iterator
             it = frames.begin(); it != frames.end(); ++it) {
        if (it->is_skip()) continue;
        if (it->is_head()) {
            rassert(it == frames.begin());
            continue;
        }
        *bt->add_frames() = it->toproto();
    }
}
void backtrace_t::fill_error(Response *res, Response_ResponseType type,
                             std::string msg) const {
    guarantee(type == Response::CLIENT_ERROR ||
              type == Response::COMPILE_ERROR ||
              type == Response::RUNTIME_ERROR);
    Datum error_msg;
    error_msg.set_type(Datum::R_STR);
    error_msg.set_r_str(msg);
    res->set_type(type);
    *res->add_response() = error_msg;
    fill_bt(res->mutable_backtrace());
}
void fill_error(Response *res, Response_ResponseType type, std::string msg,
                const backtrace_t &bt) {
    bt.fill_error(res, type, msg);
}

Frame backtrace_t::frame_t::toproto() const {
    Frame f;
    switch(type) {
    case POS: {
        f.set_type(Frame_FrameType_POS);
        f.set_pos(pos);
    } break;
    case OPT: {
        f.set_type(Frame_FrameType_OPT);
        f.set_opt(opt);
    } break;
    default: unreachable();
    }
    return f;
}

backtrace_t::frame_t::frame_t(const Frame &f) {
    switch(f.type()) {
    case Frame::POS: {
        type = POS;
        pos = f.pos();
    } break;
    case Frame::OPT: {
        type = OPT;
        opt = f.opt();
    } break;
    default: unreachable();
    }
}

void pb_rcheckable_t::propagate(Term *t) const {
    // SAMRSI: Make sure that term_walker_t _copies_ from bt_src.
    term_walker_t(t, bt_src.get());
}

} // namespace ql
