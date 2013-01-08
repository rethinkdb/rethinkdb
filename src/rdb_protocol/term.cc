// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/term.hpp"

#include "rdb_protocol/terms/arith.hpp"
#include "rdb_protocol/terms/datum_terms.hpp"
#include "rdb_protocol/terms/db_table.hpp"
#include "rdb_protocol/terms/error.hpp"
#include "rdb_protocol/terms/map.hpp"
#include "rdb_protocol/terms/pred.hpp"
#include "rdb_protocol/terms/var.hpp"

namespace ql {

term_t *compile_term(env_t *env, const Term2 *t) {
    switch(t->type()) {
    case Term2_TermType_DATUM: return new datum_term_t(env, &t->datum());
    case Term2_TermType_MAKE_ARRAY: return new make_array_term_t(env, t);
    case Term2_TermType_MAKE_OBJ: return new make_obj_term_t(env, t);
    case Term2_TermType_VAR: return new var_term_t(env, t);
    case Term2_TermType_JAVASCRIPT:
        throw ql::exc_t("JAVASCRIPT UNIMPLEMENTED (Bill's job.)");
    case Term2_TermType_ERROR: return new error_term_t(env, t);
    case Term2_TermType_IMPLICIT_VAR: return new implicit_var_term_t(env, t);
    case Term2_TermType_DB: return new db_term_t(env, t);
    case Term2_TermType_TABLE: return new table_term_t(env, t);
    case Term2_TermType_GET:
        break;
    case Term2_TermType_EQ: // fallthru
    case Term2_TermType_NE: // fallthru
    case Term2_TermType_LT: // fallthru
    case Term2_TermType_LE: // fallthru
    case Term2_TermType_GT: // fallthru
    case Term2_TermType_GE: return new predicate_term_t(env, t);
    case Term2_TermType_NOT: return new not_term_t(env, t);
    case Term2_TermType_ADD: // fallthru
    case Term2_TermType_SUB: // fallthru
    case Term2_TermType_MUL: // fallthru
    case Term2_TermType_DIV: return new arith_term_t(env, t);
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
        break;
    case Term2_TermType_MAP:
        return new map_term_t(env, t);
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
        break;
    case Term2_TermType_FUNC: return new func_term_t(env, t);
    default: unreachable();
    }
    rfail("UNIMPLEMENTED %p", env);
    unreachable();
}

void run(Query2 *q, env_t *env, Response2 *res, stream_cache_t *stream_cache) {
    switch(q->type()) {
    case Query2_QueryType_START: {
        term_t *root_term = 0;
        try {
            root_term = env->new_term(&q->query());
        } catch (const exc_t &e) {
            fill_error(res, Response2::COMPILE_ERROR, e.what(), e.backtrace);
            return;
        }

        try {
            val_t *val = root_term->eval(env);
            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response2_ResponseType_SUCCESS_ATOM);
                const datum_t *d = val->as_datum();
                d->write_to_protobuf(res->add_response());
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                // TODO: SUCCESS_PARTIAL
                res->set_type(Response2_ResponseType_SUCCESS_SEQUENCE);
                datum_stream_t *ds = val->as_seq();
                for (;;) {
                    ql::env_checkpointer_t(env, &ql::env_t::discard_checkpoint);
                    const datum_t *d = ds->next();
                    if (!d) break;
                    d->write_to_protobuf(res->add_response());
                }
            }
        } catch (const exc_t &e) {
            fill_error(res, Response2::RUNTIME_ERROR, e.what(), e.backtrace);
            return;
        }
    }; break;
    case Query2_QueryType_CONTINUE: {
        try {
            rfail("UNIMPLEMENTED %p", stream_cache);
        } catch (const exc_t &e) {
            fill_error(res, Response2::CLIENT_ERROR, e.what(), e.backtrace);
            return;
        }
    }; break;
    case Query2_QueryType_STOP: {
        try {
            rfail("UNIMPLEMENTED");
        } catch (const exc_t &e) {
            fill_error(res, Response2::CLIENT_ERROR, e.what(), e.backtrace);
            return;
        }
    }; break;
    default: unreachable();
    }
}

term_t::term_t(env_t *_env) : use_cached_val(false), env(_env), cached_val(0) {
    guarantee(env);
}
term_t::~term_t() { }
val_t *term_t::eval(bool _use_cached_val) {
    use_cached_val = _use_cached_val;
    try {
        if (!cached_val || !use_cached_val) cached_val = eval_impl();
    } catch (exc_t &e) {
        if (has_bt()) e.backtrace.frames.push_front(get_bt());
        throw;
    }
    return cached_val;
}

val_t *term_t::new_val(datum_t *d) { return env->new_val(d, this); }
val_t *term_t::new_val(const datum_t *d) { return env->new_val(d, this); }
val_t *term_t::new_val(datum_stream_t *s) { return env->new_val(s, this); }
val_t *term_t::new_val(uuid_t db) { return env->new_val(db, this); }
val_t *term_t::new_val(table_t *t) { return env->new_val(t, this); }
val_t *term_t::new_val(func_t *f) { return env->new_val(f, this); }

// val_t *term_t::new_val(datum_t *d) { return env->new_val(d, this); }
// val_t *term_t::new_val(datum_stream_t *s) { return env->new_val(s, this); }
// val_t *term_t::new_val(uuid_t db) { return env->add_and_ret(db, this); }
// val_t *term_t::new_val(table_t *t) { return env->add_and_ret(t, this); }
// val_t *term_t::new_val(func_t *f) { return env->add_and_ret(f, this); }

} //namespace ql
