// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/ql2.hpp"

namespace ql {

func_t::func_t(env_t *env, const std::vector<int> &args, const Term2 *body_source) {
    for (size_t i = 0; i < args.size(); ++i) {
        argptrs.push_back(0);
        env->push_var(args[i], &argptrs[i]);
    }
    body.init(compile_term(env, body_source));
    for (size_t i = 0; i < args.size(); ++i) env->pop_var(args[i]);
}

val_t *func_t::call(env_t *env, const std::vector<datum_t *> &args) {
    runtime_check(args.size() == argptrs.size(),
                  strprintf("Passed %lu arguments to function of arity %lu.",
                            args.size(), argptrs.size()));
    for (size_t i = 0; i < args.size(); ++i) argptrs[i] = args[i];
    return body->eval(env, false);
    //                ^^^^^ don't use cached value
}

val_t::type_t::type_t(val_t::type_t::raw_type_t _raw_type) : raw_type(_raw_type) { }

bool raw_type_is_convertible(val_t::type_t::raw_type_t _t1,
                             val_t::type_t::raw_type_t _t2) {
    const int t1 = _t1, t2 = _t2,
        DB = val_t::type_t::DB,
        TABLE = val_t::type_t::TABLE,
        SELECTION = val_t::type_t::SELECTION,
        SEQUENCE = val_t::type_t::SEQUENCE,
        SINGLE_SELECTION = val_t::type_t::SINGLE_SELECTION,
        DATUM = val_t::type_t::DATUM,
        FUNC = val_t::type_t::FUNC;
    switch(t1) {
    case DB: return t2 == DB;
    case TABLE: return t2 == TABLE || t2 == SELECTION || t2 == SEQUENCE;
    case SELECTION: return t2 == SELECTION || t2 == SEQUENCE;
    case SEQUENCE: return t2 == SEQUENCE;
    case SINGLE_SELECTION: return t2 == SINGLE_SELECTION || t2 == DATUM;
    case DATUM: return t2 == DATUM || t2 == SEQUENCE;
    case FUNC: return t2 == FUNC;
    default: unreachable();
    }
    unreachable();
}
bool val_t::type_t::is_convertible(type_t rhs) const {
    return raw_type_is_convertible(raw_type, rhs.raw_type);
}

const char *val_t::type_t::name() const {
    switch(raw_type) {
    case DB: return "DATABASE";
    case TABLE: return "TABLE";
    case SELECTION: return "SELECTION";
    case SEQUENCE: return "SEQUENCE";
    case SINGLE_SELECTION: return "SINGLE_SELECTION";
    case DATUM: return "DATUM";
    case FUNC: return "FUNCTION";
    default: unreachable();
    }
    unreachable();
}

val_t::val_t(const datum_t *_datum) : type(type_t::DATUM), datum(_datum) {
    guarantee(_datum);
}

val_t::type_t val_t::get_type() const { return type; }

const datum_t *val_t::as_datum() {
    runtime_check(type.raw_type == type_t::DATUM,
                  strprintf("Type error: cannot convert %s to DATUM.", type.name()));
    return datum.get();
}

} //namespace ql
