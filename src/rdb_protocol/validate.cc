// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "utils.hpp"

#include "rdb_protocol/validate.hpp"
#include "rdb_protocol/error.hpp"

#define check_has(pb, has_field, field) do {                            \
        rcheck_toplevel(                                                \
            (pb).has_field(),                                           \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (missing field `%s`):\n%s",   \
                      field, (pb).DebugString().c_str()));              \
    } while (0)

#define check_not_has(pb, has_field, field) do {                        \
        rcheck_toplevel(                                                \
            !(pb).has_field(),                                          \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (spurious field `%s`):\n%s",  \
                      field, (pb).DebugString().c_str()));              \
    } while (0)

#define check_empty(pb, field_size, field) do {                         \
        rcheck_toplevel(                                                \
            (pb).field_size() == 0,                                     \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (non-empty field `%s`):\n%s", \
                      field, (pb).DebugString().c_str()));              \
    } while (0)

void validate_pb(const Query &q) {
    check_has(q, has_type, "type");
    if (q.type() == Query::START) {
        check_has(q, has_query, "query");
        validate_pb(q.query());
    } else {
        check_not_has(q, has_query, "query");
    }
    check_has(q, has_token, "token");
    for (int i = 0; i < q.global_optargs_size(); ++i) {
        validate_pb(q.global_optargs(i).val());
    }
}

void validate_pb(const Query::AssocPair &ap) {
    check_has(ap, has_key, "key");
    check_has(ap, has_val, "val");
    validate_pb(ap.val());
}

void validate_pb(const Frame &f) {
    check_has(f, has_type, "type");
    if (f.type() == Frame::POS) {
        check_has(f, has_pos, "pos");
    } else {
        check_has(f, has_opt, "opt");
    }
}

void validate_pb(const Backtrace &bt) {
    for (int i = 0; i < bt.frames_size(); ++i) {
        validate_pb(bt.frames(i));
    }
}

void validate_pb(const Response &r) {
    check_has(r, has_type, "type");
    if (r.type() == Response::SUCCESS_ATOM
        || r.type() == Response::SUCCESS_SEQUENCE
        || r.type() == Response::SUCCESS_PARTIAL) {
        check_not_has(r, has_backtrace, "backtrace");
    } else {
        check_has(r, has_backtrace, "backtrace");
    }
    check_has(r, has_token, "token");
    for (int i = 0; i < r.response_size(); ++i) {
        validate_pb(r.response(i));
    }
}

void validate_pb(const Datum &d) {
    check_has(d, has_type, "type");
    if (d.type() == Datum::R_BOOL) {
        check_has(d, has_r_bool, "r_bool");
    } else {
        check_not_has(d, has_r_bool, "r_bool");
    }
    if (d.type() == Datum::R_NUM) {
        check_has(d, has_r_num, "r_num");
    } else {
        check_not_has(d, has_r_num, "r_num");
    }
    if (d.type() == Datum::R_STR) {
        check_has(d, has_r_str, "r_str");
    } else {
        check_not_has(d, has_r_str, "r_str");
    }
    if (d.type() == Datum::R_ARRAY) {
        for (int i = 0; i < d.r_array_size(); ++i) {
            validate_pb(d.r_array(i));
        }
    } else {
        check_empty(d, r_array_size, "r_array");
    }
    if (d.type() == Datum::R_OBJECT) {
        for (int i = 0; i < d.r_object_size(); ++i) {
            validate_pb(d.r_object(i));
        }
    } else {
        check_empty(d, r_object_size, "r_object");
    }
}

void validate_pb(const Datum::AssocPair &ap) {
    check_has(ap, has_key, "key");
    check_has(ap, has_val, "val");
    validate_pb(ap.val());
}

void validate_pb(const Term &t) {
    check_has(t, has_type, "type");
    if (t.type() == Term::DATUM) {
        check_has(t, has_datum, "datum");
    } else {
        check_not_has(t, has_datum, "datum");
    }
    for (int i = 0; i < t.args_size(); ++i) {
        validate_pb(t.args(i));
    }
    for (int i = 0; i < t.optargs_size(); ++i) {
        validate_pb(t.optargs(i));
    }
}

void validate_pb(const Term::AssocPair &ap) {
    check_has(ap, has_key, "key");
    check_has(ap, has_val, "val");
    validate_pb(ap.val());
}
