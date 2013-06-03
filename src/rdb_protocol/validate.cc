#include "utils.hpp"

#include "rdb_protocol/validate.hpp"
#include "rdb_protocol/error.hpp"

#define check_has(pb, field)                                            \
    rcheck_toplevel(                                                    \
        (pb).has_##field(),                                             \
        ql::base_exc_t::GENERIC,                                \
        strprintf("MALFORMED PROTOBUF (missing field `%s`):\n%s",       \
                  #field, (pb).DebugString().c_str()))
#define check_not_has(pb, field)                                        \
    rcheck_toplevel(                                                    \
        !(pb).has_##field(),                                            \
        ql::base_exc_t::GENERIC,                                \
        strprintf("MALFORMED PROTOBUF (spurious field `%s`):\n%s",      \
                  #field, (pb).DebugString().c_str()))
#define check_empty(pb, field)                                          \
    rcheck_toplevel(                                                    \
        (pb).field##_size() == 0,                                       \
        ql::base_exc_t::GENERIC,                                \
        strprintf("MALFORMED PROTOBUF (non-empty field `%s`):\n%s",     \
                  #field, (pb).DebugString().c_str()))

void validate_pb(const Query &q) {
    check_has(q, type);
    if (q.type() == Query::START) {
        check_has(q, query);
        validate_pb(q.query());
    } else {
        check_not_has(q, query);
    }
    check_has(q, token);
    for (int i = 0; i < q.global_optargs_size(); ++i) {
        validate_pb(q.global_optargs(i).val());
    }
}

void validate_pb(const Query::AssocPair &ap) {
    check_has(ap, key);
    check_has(ap, val);
    validate_pb(ap.val());
}

void validate_pb(const Frame &f) {
    check_has(f, type);
    if (f.type() == Frame::POS) {
        check_has(f, pos);
    } else {
        check_has(f, opt);
    }
}

void validate_pb(const Backtrace &bt) {
    for (int i = 0; i < bt.frames_size(); ++i) {
        validate_pb(bt.frames(i));
    }
}

void validate_pb(const Response &r) {
    check_has(r, type);
    if (r.type() == Response::SUCCESS_ATOM
        || r.type() == Response::SUCCESS_SEQUENCE
        || r.type() == Response::SUCCESS_PARTIAL) {
        check_not_has(r, backtrace);
    } else {
        check_has(r, backtrace);
    }
    check_has(r, token);
    for (int i = 0; i < r.response_size(); ++i) {
        validate_pb(r.response(i));
    }
}

void validate_pb(const Datum &d) {
    check_has(d, type);
    if (d.type() == Datum::R_BOOL) {
        check_has(d, r_bool);
    } else {
        check_not_has(d, r_bool);
    }
    if (d.type() == Datum::R_NUM) {
        check_has(d, r_num);
    } else {
        check_not_has(d, r_num);
    }
    if (d.type() == Datum::R_STR) {
        check_has(d, r_str);
    } else {
        check_not_has(d, r_str);
    }
    if (d.type() == Datum::R_ARRAY) {
        for (int i = 0; i < d.r_array_size(); ++i) {
            validate_pb(d.r_array(i));
        }
    } else {
        check_empty(d, r_array);
    }
    if (d.type() == Datum::R_OBJECT) {
        for (int i = 0; i < d.r_object_size(); ++i) {
            validate_pb(d.r_object(i));
        }
    } else {
        check_empty(d, r_object);
    }
}

void validate_pb(const Datum::AssocPair &ap) {
    check_has(ap, key);
    check_has(ap, val);
    validate_pb(ap.val());
}

void validate_pb(const Term &t) {
    check_has(t, type);
    if (t.type() == Term::DATUM) {
        check_has(t, datum);
    } else {
        check_not_has(t, datum);
    }
    for (int i = 0; i < t.args_size(); ++i) {
        validate_pb(t.args(i));
    }
    for (int i = 0; i < t.optargs_size(); ++i) {
        validate_pb(t.optargs(i));
    }
}

void validate_pb(const Term::AssocPair &ap) {
    check_has(ap, key);
    check_has(ap, val);
    validate_pb(ap.val());
}
