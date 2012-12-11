// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/ql2.hpp"

namespace ql {

term_t *compile_term(env_t *env, const Term2 *t) {
    switch(t->type()) {
    case Term2_TermType_DATUM: {
        return new datum_term_t(&t->datum());
    }; break;
    case Term2_TermType_MAKE_ARRAY:
    case Term2_TermType_MAKE_OBJ:
    case Term2_TermType_VAR:
    case Term2_TermType_JAVASCRIPT:
    case Term2_TermType_ERROR:
    case Term2_TermType_IMPLICIT_VAR:
    case Term2_TermType_DB:
    case Term2_TermType_TABLE:
    case Term2_TermType_GET:
    case Term2_TermType_EQ:
    case Term2_TermType_NE:
    case Term2_TermType_LT:
    case Term2_TermType_LE:
    case Term2_TermType_GT:
    case Term2_TermType_GE:
    case Term2_TermType_NOT:
    case Term2_TermType_ADD:
    case Term2_TermType_SUB:
    case Term2_TermType_MUL:
    case Term2_TermType_DIV:
    case Term2_TermType_MOD:
    case Term2_TermType_APPEND:
    case Term2_TermType_SLICE:
    case Term2_TermType_GETATTR:
    case Term2_TermType_CONTAINS:
    case Term2_TermType_PLUCK:
    case Term2_TermType_WITHOUT:
    case Term2_TermType_MERGE:
    case Term2_TermType_BETWEEN:
    case Term2_TermType_REDUCE:
    case Term2_TermType_MAP:
    case Term2_TermType_FILTER:
    case Term2_TermType_CONCATMAP:
    case Term2_TermType_ORDERBY:
    case Term2_TermType_DISTINCT:
    case Term2_TermType_COUNT:
    case Term2_TermType_UNION:
    case Term2_TermType_NTH:
    case Term2_TermType_GROUPED_MAP_REDUCE:
    case Term2_TermType_GROUPBY:
    case Term2_TermType_INNER_JOIN:
    case Term2_TermType_OUTER_JOIN:
    case Term2_TermType_EQ_JOIN:
    case Term2_TermType_COERCE:
    case Term2_TermType_TYPEOF:
    case Term2_TermType_UPDATE:
    case Term2_TermType_DELETE:
    case Term2_TermType_REPLACE:
    case Term2_TermType_INSERT:
    case Term2_TermType_DB_CREATE:
    case Term2_TermType_DB_DROP:
    case Term2_TermType_DB_LIST:
    case Term2_TermType_TABLE_CREATE:
    case Term2_TermType_TABLE_DROP:
    case Term2_TermType_TABLE_LIST:
    case Term2_TermType_FUNCALL:
    case Term2_TermType_BRANCH:
    case Term2_TermType_ANY:
    case Term2_TermType_ALL:
    case Term2_TermType_FOREACH:
    case Term2_TermType_FUNC:
    default: break;
    }
    runtime_check(false, strprintf("UNIMPLEMENTED %p", env));
    unreachable();
}

void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache) {
    switch(q->type()) {
    case Query2_QueryType_START: {
        scoped_ptr_t<term_t> root_term(compile_term(env, &q->query()));
        val_t *val = root_term->eval(env);
        if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
            const datum_t *d = val->as_datum();
            res->set_type(Response2_ResponseType_SUCCESS_ATOM);
            d->write_to_protobuf(res->add_response());
        } else {
            runtime_check(false, "UNIMPLEMENTED");
        }
    }; break;
    case Query2_QueryType_CONTINUE: {
        runtime_check(false, strprintf("UNIMPLEMENTED %p", stream_cache));
    }; break;
    case Query2_QueryType_STOP: {
        runtime_check(false, "UNIMPLEMENTED");
    }; break;
    default: unreachable();
    }
}

term_t::term_t() : cached_val(0) { }
term_t::~term_t() { }
val_t *term_t::eval(env_t *env, bool use_cached_val) {
    if (!cached_val || !use_cached_val) cached_val = eval_impl(env);
    return cached_val;
}


} //namespace ql
