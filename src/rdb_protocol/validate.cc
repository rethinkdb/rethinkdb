// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/error.hpp"
#include "rdb_protocol/validate.hpp"
#include "utils.hpp"


#define check_has(pb, has_field, field) do {                            \
        auto const &check_has_tmp = (pb);                               \
        rcheck_toplevel(                                                \
            check_has_tmp.has_field(),                                  \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (missing field `%s`):\n%s",   \
                      (field), check_has_tmp.DebugString().c_str()));   \
    } while (0)

#define check_not_has(pb, has_field, field) do {                        \
        auto const &check_not_has_tmp = (pb);                           \
        rcheck_toplevel(                                                \
            !check_not_has_tmp.has_field(),                             \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (spurious field `%s`):\n%s",  \
                      (field),                                          \
                      check_not_has_tmp.DebugString().c_str()));        \
    } while (0)

#define check_size(expected_size, pb, field_size, field) do {           \
        auto const &check_size_tmp = (pb);                              \
        const int check_size_expected = (expected_size);                \
        rcheck_toplevel(                                                \
            check_size_tmp.field_size() == check_size_expected,         \
            ql::base_exc_t::GENERIC,                                    \
            strprintf("MALFORMED PROTOBUF (expected field `%s` "        \
                      "to have size %d):\n%s",                          \
                      (field), check_size_expected,                     \
                      check_size_tmp.DebugString().c_str()));           \
    } while(0)

#define check_empty(pb, field_size, field) check_size(0, pb, field_size, field)

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
    if (d.type() == Datum::R_STR || d.type() == Datum::R_JSON) {
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

void validate_var_term(const Term &t) {
    check_empty(t, optargs_size, "optargs");
    check_size(1, t, args_size, "args");
    const Term &arg0 = t.args(0);
    rcheck_toplevel(arg0.type() == Term::DATUM && arg0.has_datum() &&
                    arg0.datum().type() == Datum::R_NUM &&
                    arg0.datum().has_r_num(),
                    ql::base_exc_t::GENERIC,
                    strprintf("MALFORMED PROTOBUF (expected VAR term "
                              "to have DATUM of type R_NUM):\n%s",
                              t.DebugString().c_str()));
    double number = arg0.datum().r_num();
    int64_t i;
    if (!ql::number_as_integer(number, &i) || i < 0) {
        rfail_toplevel(ql::base_exc_t::GENERIC,
                       "MALFORMED PROTOBUF (VAR term should have "
                       "a positive integer value):\n%s",
                       t.DebugString().c_str());
    }
}

void validate_pb(const Term &t) {
    check_has(t, has_type, "type");
    if (t.type() == Term::DATUM) {
        check_has(t, has_datum, "datum");
    } else {
        check_not_has(t, has_datum, "datum");
        if (t.type() == Term::VAR) {
            validate_var_term(t);
            return;
        }
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
