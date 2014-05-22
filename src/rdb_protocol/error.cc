// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/error.hpp"

#include "backtrace.hpp"
#include "containers/archive/stl_types.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/val.hpp"

namespace ql {

#ifdef RQL_ERROR_BT
#define RQL_ERROR_VAR
#else
#define RQL_ERROR_VAR UNUSED
#endif

void runtime_fail(base_exc_t::type_t type,
                  RQL_ERROR_VAR const char *test, RQL_ERROR_VAR const char *file,
                  RQL_ERROR_VAR int line,
                  std::string msg, const Backtrace *bt_src) {
#ifdef RQL_ERROR_BT
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw exc_t(type, msg, bt_src);
}
void runtime_fail(base_exc_t::type_t type,
                  RQL_ERROR_VAR const char *test, RQL_ERROR_VAR const char *file,
                  RQL_ERROR_VAR int line, std::string msg) {
#ifdef RQL_ERROR_BT
    msg = strprintf("%s\nFailed assertion: %s\nAt: %s:%d",
                    msg.c_str(), test, file, line);
#endif
    throw datum_exc_t(type, msg);
}

void runtime_sanity_check_failed() {
    lazy_backtrace_formatter_t bt;
    throw exc_t(base_exc_t::GENERIC,
                "SANITY CHECK FAILED (server is buggy).  Backtrace:\n" + bt.addrs(),
                backtrace_t());
}

base_exc_t::type_t exc_type(const datum_t *d) {
    r_sanity_check(d);
    return d->get_type() == datum_t::R_NULL
        ? base_exc_t::NON_EXISTENCE
        : base_exc_t::GENERIC;
}
base_exc_t::type_t exc_type(const counted_t<const datum_t> &d) {
    r_sanity_check(d.has());
    return exc_type(d.get());
}
base_exc_t::type_t exc_type(const val_t *v) {
    r_sanity_check(v);
    if (v->get_type().is_convertible(val_t::type_t::DATUM)) {
        return exc_type(v->as_datum());
    } else {
        return base_exc_t::GENERIC;
    }
}
base_exc_t::type_t exc_type(const counted_t<val_t> &v) {
    r_sanity_check(v.has());
    return exc_type(v.get());
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

protob_t<const Backtrace> get_backtrace(const protob_t<const Term> &t) {
    return t.make_child(&t->GetExtension(ql2::extension::backtrace));
}

void pb_rcheckable_t::propagate(Term *t) const {
    propagate_backtrace(t, bt_src.get());
}

RDB_IMPL_ME_SERIALIZABLE_1(backtrace_t, 0, frames);
RDB_IMPL_ME_SERIALIZABLE_3(backtrace_t::frame_t, 0, type, pos, opt);
RDB_IMPL_ME_SERIALIZABLE_3(exc_t, 0, type_, backtrace_, exc_msg_);
RDB_IMPL_ME_SERIALIZABLE_2(datum_exc_t, 0, type_, exc_msg);


} // namespace ql
