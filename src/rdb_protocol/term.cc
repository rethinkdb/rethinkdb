// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"

#include "rdb_protocol/terms/arith.hpp"
#include "rdb_protocol/terms/arr.hpp"
#include "rdb_protocol/terms/control.hpp"
#include "rdb_protocol/terms/datum_terms.hpp"
#include "rdb_protocol/terms/db_table.hpp"
#include "rdb_protocol/terms/error.hpp"
#include "rdb_protocol/terms/gmr.hpp"
#include "rdb_protocol/terms/obj.hpp"
#include "rdb_protocol/terms/obj_or_seq.hpp"
#include "rdb_protocol/terms/pred.hpp"
#include "rdb_protocol/terms/rewrites.hpp"
#include "rdb_protocol/terms/seq.hpp"
#include "rdb_protocol/terms/sort.hpp"
#include "rdb_protocol/terms/type_manip.hpp"
#include "rdb_protocol/terms/var.hpp"
#include "rdb_protocol/terms/writes.hpp"

namespace ql {

term_t *compile_term(env_t *env, const Term2 *t) {
    switch(t->type()) {
    case Term2_TermType_DATUM:              return new datum_term_t(env, &t->datum());
    case Term2_TermType_MAKE_ARRAY:         return new make_array_term_t(env, t);
    case Term2_TermType_MAKE_OBJ:           return new make_obj_term_t(env, t);
    case Term2_TermType_VAR:                return new var_term_t(env, t);
    case Term2_TermType_JAVASCRIPT:
        throw exc_t("JAVASCRIPT UNIMPLEMENTED (Bill's job.)");
    case Term2_TermType_ERROR:              return new error_term_t(env, t);
    case Term2_TermType_IMPLICIT_VAR:       return new implicit_var_term_t(env, t);
    case Term2_TermType_DB:                 return new db_term_t(env, t);
    case Term2_TermType_TABLE:              return new table_term_t(env, t);
    case Term2_TermType_GET:                return new get_term_t(env, t);
    case Term2_TermType_EQ:                 // fallthru
    case Term2_TermType_NE:                 // fallthru
    case Term2_TermType_LT:                 // fallthru
    case Term2_TermType_LE:                 // fallthru
    case Term2_TermType_GT:                 // fallthru
    case Term2_TermType_GE:                 return new predicate_term_t(env, t);
    case Term2_TermType_NOT:                return new not_term_t(env, t);
    case Term2_TermType_ADD:                // fallthru
    case Term2_TermType_SUB:                // fallthru
    case Term2_TermType_MUL:                // fallthru
    case Term2_TermType_DIV:                return new arith_term_t(env, t);
    case Term2_TermType_MOD:                return new mod_term_t(env, t);
    case Term2_TermType_APPEND:             return new append_term_t(env, t);
    case Term2_TermType_SLICE:              return new slice_term_t(env, t);
    case Term2_TermType_GETATTR:            return new getattr_term_t(env, t);
    case Term2_TermType_CONTAINS:           return new contains_term_t(env, t);
    case Term2_TermType_PLUCK:              return new pluck_term_t(env, t);
    case Term2_TermType_WITHOUT:            return new without_term_t(env, t);
    case Term2_TermType_MERGE:              return new merge_term_t(env, t);
    case Term2_TermType_BETWEEN:            return new between_term_t(env, t);
    case Term2_TermType_REDUCE:             return new reduce_term_t(env, t);
    case Term2_TermType_MAP:                return new map_term_t(env, t);
    case Term2_TermType_FILTER:             return new filter_term_t(env, t);
    case Term2_TermType_CONCATMAP:          return new concatmap_term_t(env, t);
    case Term2_TermType_ORDERBY:            return new orderby_term_t(env, t);
    case Term2_TermType_DISTINCT:           return new distinct_term_t(env, t);
    case Term2_TermType_COUNT:              return new count_term_t(env, t);
    case Term2_TermType_UNION:              return new union_term_t(env, t);
    case Term2_TermType_NTH:                return new nth_term_t(env, t);
    case Term2_TermType_GROUPED_MAP_REDUCE: return new gmr_term_t(env, t);
    case Term2_TermType_LIMIT:              return new limit_term_t(env, t);
    case Term2_TermType_SKIP:               return new skip_term_t(env, t);
    case Term2_TermType_GROUPBY:
        rfail("UNIMPLEMENTED");
    case Term2_TermType_INNER_JOIN:         return new inner_join_term_t(env, t);
    case Term2_TermType_OUTER_JOIN:         return new outer_join_term_t(env, t);
    case Term2_TermType_EQ_JOIN:            return new eq_join_term_t(env, t);
    case Term2_TermType_COERCE:             return new coerce_term_t(env, t);
    case Term2_TermType_TYPEOF:             return new typeof_term_t(env, t);
    case Term2_TermType_UPDATE:             return new update_term_t(env, t);
    case Term2_TermType_DELETE:             return new delete_term_t(env, t);
    case Term2_TermType_REPLACE:            return new replace_term_t(env, t);
    case Term2_TermType_INSERT:             return new insert_term_t(env, t);
    case Term2_TermType_DB_CREATE:          return new db_create_term_t(env, t);
    case Term2_TermType_DB_DROP:            return new db_drop_term_t(env, t);
    case Term2_TermType_DB_LIST:            return new db_list_term_t(env, t);
    case Term2_TermType_TABLE_CREATE:       return new table_create_term_t(env, t);
    case Term2_TermType_TABLE_DROP:         return new table_drop_term_t(env, t);
    case Term2_TermType_TABLE_LIST:         return new table_list_term_t(env, t);
    case Term2_TermType_FUNCALL:            return new funcall_term_t(env, t);
    case Term2_TermType_BRANCH:             return new branch_term_t(env, t);
    case Term2_TermType_ANY:                return new any_term_t(env, t);
    case Term2_TermType_ALL:                return new all_term_t(env, t);
    case Term2_TermType_FOREACH:            return new foreach_term_t(env, t);
    case Term2_TermType_FUNC:               return new func_term_t(env, t);
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
            val_t *val = root_term->eval(false);
            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response2_ResponseType_SUCCESS_ATOM);
                const datum_t *d = val->as_datum();
                d->write_to_protobuf(res->add_response());
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                // TODO: SUCCESS_PARTIAL
                res->set_type(Response2_ResponseType_SUCCESS_SEQUENCE);
                datum_stream_t *ds = val->as_seq();
                for (;;) {
                    env_checkpoint_t(env, &env_t::discard_checkpoint);
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

//#define INSTRUMENT 1
#ifdef INSTRUMENT
__thread int depth = 0;
#endif //INSTRUMENT

val_t *term_t::eval(bool _use_cached_val) {
#ifdef INSTRUMENT
    std::string s = "";
    for (int i = 0; i < depth; ++i) s += " ";
    debugf("%sEVALUATING %s:\n", s.c_str(), name());
    ++depth;
#endif // INSTRUMENT

    use_cached_val = _use_cached_val;
    try {
        if (!cached_val || !use_cached_val) cached_val = eval_impl();
    } catch (exc_t &e) {
#ifdef INSTRUMENT
        --depth;
        debugf("%s%s THREW\n", s.c_str(), name());
#endif // INSTRUMENT
        if (has_bt()) e.backtrace.frames.push_front(get_bt());
        throw;
    }

#ifdef INSTRUMENT
    --depth;
    debugf("%s%s returned %s\n", s.c_str(), name(), cached_val->print().c_str());
#endif // INSTRUMENT
    return cached_val;
}

val_t *term_t::new_val(datum_t *d) {
    return new_val(const_cast<const datum_t *>(d));
}
val_t *term_t::new_val(const datum_t *d) {
    return env->new_val(d, this);
}
val_t *term_t::new_val(datum_t *d, table_t *t) {
    return new_val(const_cast<const datum_t *>(d), t);
}
val_t *term_t::new_val(const datum_t *d, table_t *t) {
    return env->new_val(d, t, this);
}

val_t *term_t::new_val(datum_stream_t *s) { return env->new_val(s, this); }
val_t *term_t::new_val(table_t *d, datum_stream_t *s) {
    return env->new_val(d, s, this);
}
val_t *term_t::new_val(uuid_u db) { return env->new_val(db, this); }
val_t *term_t::new_val(table_t *t) { return env->new_val(t, this); }
val_t *term_t::new_val(func_t *f) { return env->new_val(f, this); }

// val_t *term_t::new_val(datum_t *d) { return env->new_val(d, this); }
// val_t *term_t::new_val(datum_stream_t *s) { return env->new_val(s, this); }
// val_t *term_t::new_val(uuid_u db) { return env->add_and_ret(db, this); }
// val_t *term_t::new_val(table_t *t) { return env->add_and_ret(t, this); }
// val_t *term_t::new_val(func_t *f) { return env->add_and_ret(f, this); }

} //namespace ql
