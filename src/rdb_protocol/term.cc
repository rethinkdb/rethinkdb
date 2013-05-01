// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/term.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/validate.hpp"

#include "rdb_protocol/terms/arith.hpp"
#include "rdb_protocol/terms/arr.hpp"
#include "rdb_protocol/terms/control.hpp"
#include "rdb_protocol/terms/datum_terms.hpp"
#include "rdb_protocol/terms/db_table.hpp"
#include "rdb_protocol/terms/error.hpp"
#include "rdb_protocol/terms/gmr.hpp"
#include "rdb_protocol/terms/js.hpp"
#include "rdb_protocol/terms/obj.hpp"
#include "rdb_protocol/terms/obj_or_seq.hpp"
#include "rdb_protocol/terms/pred.hpp"
#include "rdb_protocol/terms/rewrites.hpp"
#include "rdb_protocol/terms/seq.hpp"
#include "rdb_protocol/terms/sindex.hpp"
#include "rdb_protocol/terms/sort.hpp"
#include "rdb_protocol/terms/type_manip.hpp"
#include "rdb_protocol/terms/var.hpp"
#include "rdb_protocol/terms/writes.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

term_t *compile_term(env_t *env, const Term *t) {
    switch (t->type()) {
    case Term_TermType_DATUM:              return new datum_term_t(env, t);
    case Term_TermType_MAKE_ARRAY:         return new make_array_term_t(env, t);
    case Term_TermType_MAKE_OBJ:           return new make_obj_term_t(env, t);
    case Term_TermType_VAR:                return new var_term_t(env, t);
    case Term_TermType_JAVASCRIPT:         return new javascript_term_t(env, t);
    case Term_TermType_ERROR:              return new error_term_t(env, t);
    case Term_TermType_IMPLICIT_VAR:       return new implicit_var_term_t(env, t);
    case Term_TermType_DB:                 return new db_term_t(env, t);
    case Term_TermType_TABLE:              return new table_term_t(env, t);
    case Term_TermType_GET:                return new get_term_t(env, t);
    case Term_TermType_EQ:                 // fallthru
    case Term_TermType_NE:                 // fallthru
    case Term_TermType_LT:                 // fallthru
    case Term_TermType_LE:                 // fallthru
    case Term_TermType_GT:                 // fallthru
    case Term_TermType_GE:                 return new predicate_term_t(env, t);
    case Term_TermType_NOT:                return new not_term_t(env, t);
    case Term_TermType_ADD:                // fallthru
    case Term_TermType_SUB:                // fallthru
    case Term_TermType_MUL:                // fallthru
    case Term_TermType_DIV:                return new arith_term_t(env, t);
    case Term_TermType_MOD:                return new mod_term_t(env, t);
    case Term_TermType_APPEND:             return new append_term_t(env, t);
    case Term_TermType_SLICE:              return new slice_term_t(env, t);
    case Term_TermType_GETATTR:            return new getattr_term_t(env, t);
    case Term_TermType_CONTAINS:           return new contains_term_t(env, t);
    case Term_TermType_PLUCK:              return new pluck_term_t(env, t);
    case Term_TermType_WITHOUT:            return new without_term_t(env, t);
    case Term_TermType_MERGE:              return new merge_term_t(env, t);
    case Term_TermType_BETWEEN:            return new between_term_t(env, t);
    case Term_TermType_REDUCE:             return new reduce_term_t(env, t);
    case Term_TermType_MAP:                return new map_term_t(env, t);
    case Term_TermType_FILTER:             return new filter_term_t(env, t);
    case Term_TermType_CONCATMAP:          return new concatmap_term_t(env, t);
    case Term_TermType_ORDERBY:            return new orderby_term_t(env, t);
    case Term_TermType_DISTINCT:           return new distinct_term_t(env, t);
    case Term_TermType_COUNT:              return new count_term_t(env, t);
    case Term_TermType_UNION:              return new union_term_t(env, t);
    case Term_TermType_NTH:                return new nth_term_t(env, t);
    case Term_TermType_GROUPED_MAP_REDUCE: return new gmr_term_t(env, t);
    case Term_TermType_LIMIT:              return new limit_term_t(env, t);
    case Term_TermType_SKIP:               return new skip_term_t(env, t);
    case Term_TermType_GROUPBY:            return new groupby_term_t(env, t);
    case Term_TermType_INNER_JOIN:         return new inner_join_term_t(env, t);
    case Term_TermType_OUTER_JOIN:         return new outer_join_term_t(env, t);
    case Term_TermType_EQ_JOIN:            return new eq_join_term_t(env, t);
    case Term_TermType_ZIP:                return new zip_term_t(env, t);
    case Term_TermType_COERCE_TO:          return new coerce_term_t(env, t);
    case Term_TermType_TYPEOF:             return new typeof_term_t(env, t);
    case Term_TermType_UPDATE:             return new update_term_t(env, t);
    case Term_TermType_DELETE:             return new delete_term_t(env, t);
    case Term_TermType_REPLACE:            return new replace_term_t(env, t);
    case Term_TermType_INSERT:             return new insert_term_t(env, t);
    case Term_TermType_DB_CREATE:          return new db_create_term_t(env, t);
    case Term_TermType_DB_DROP:            return new db_drop_term_t(env, t);
    case Term_TermType_DB_LIST:            return new db_list_term_t(env, t);
    case Term_TermType_TABLE_CREATE:       return new table_create_term_t(env, t);
    case Term_TermType_TABLE_DROP:         return new table_drop_term_t(env, t);
    case Term_TermType_TABLE_LIST:         return new table_list_term_t(env, t);
    case Term_TermType_INDEX_CREATE:       return new sindex_create_term_t(env, t);
    case Term_TermType_INDEX_DROP:         return new sindex_drop_term_t(env, t);
    case Term_TermType_INDEX_LIST:         return new sindex_list_term_t(env, t);
    case Term_TermType_FUNCALL:            return new funcall_term_t(env, t);
    case Term_TermType_BRANCH:             return new branch_term_t(env, t);
    case Term_TermType_ANY:                return new any_term_t(env, t);
    case Term_TermType_ALL:                return new all_term_t(env, t);
    case Term_TermType_FOREACH:            return new foreach_term_t(env, t);
    case Term_TermType_FUNC:               return new func_term_t(env, t);
    case Term_TermType_ASC:                return new asc_term_t(env, t);
    case Term_TermType_DESC:               return new desc_term_t(env, t);
    case Term_TermType_PKEY:               return new pkey_term_t(env, t);
    default: unreachable();
    }
    unreachable();
}

void run(Query *q, scoped_ptr_t<env_t> *env_ptr,
         Response *res, stream_cache2_t *stream_cache2) {
    try {
        validate_pb(*q);
    } catch (const base_exc_t &e) {
        fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
        return;
    }
#ifdef INSTRUMENT
    debugf("Query: %s\n", q->DebugString().c_str());
#endif // INSTRUMENT
    env_t *env = env_ptr->get();
    int64_t token = q->token();

    switch (q->type()) {
    case Query_QueryType_START: {
        term_t *root_term = 0;
        try {
            Term *t = q->mutable_query();
            term_walker_t term_walker(t); // fill backtraces
            Backtrace *t_bt = t->MutableExtension(ql2::extension::backtrace);

            // Parse global optargs
            for (int i = 0; i < q->global_optargs_size(); ++i) {
                const Query::AssocPair &ap = q->global_optargs(i);
                bool conflict = env->add_optarg(ap.key(), ap.val());
                rcheck_toplevel(
                    !conflict,
                    strprintf("Duplicate global optarg: %s", ap.key().c_str()));
            }
            env_wrapper_t<Term> *ewt = env->add_ptr(new env_wrapper_t<Term>());
            Term *arg = &ewt->t;

            N1(DB, NDATUM("test"));

            term_walker_t(arg, t_bt); // duplicate toplevel backtrace
            UNUSED bool _b = env->add_optarg("db", *arg);
            //          ^^ UNUSED because user can override this value safely

            // Parse actual query
            root_term = env->new_term(t);
            // TODO: handle this properly
        } catch (const exc_t &e) {
            fill_error(res, Response::COMPILE_ERROR, e.what(), e.backtrace);
            return;
        }

        try {
            rcheck_toplevel(!stream_cache2->contains(token),
                strprintf("ERROR: duplicate token %" PRIi64, token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace);
            return;
        }

        try {
            val_t *val = root_term->eval();
            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response_ResponseType_SUCCESS_ATOM);
                const datum_t *d = val->as_datum();
                d->write_to_protobuf(res->add_response());
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                stream_cache2->insert(q, token, env_ptr, val->as_seq());
                bool b = stream_cache2->serve(token, res, env->interruptor);
                r_sanity_check(b);
            } else {
                rfail_toplevel("Query result must be of type DATUM or STREAM "
                               "(got %s).", val->get_type().name());
            }
        } catch (const exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), e.backtrace);
            return;
        }
    } break;
    case Query_QueryType_CONTINUE: {
        try {
            bool b = stream_cache2->serve(token, res, env->interruptor);
            rcheck_toplevel(b, strprintf("Token %" PRIi64 " not in stream cache.",
                                         token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace);
            return;
        }
    } break;
    case Query_QueryType_STOP: {
        try {
            rcheck_toplevel(stream_cache2->contains(token),
                strprintf("Token %" PRIi64 " not in stream cache.", token));
            stream_cache2->erase(token);
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace);
            return;
        }
    } break;
    default: unreachable();
    }
}

term_t::term_t(env_t *_env, const Term *_src)
    : pb_rcheckable_t(_src), env(_env), src(_src) {
    guarantee(env);
}
term_t::~term_t() { }

// Uncomment the define to enable instrumentation (you'll be able to see where
// you are in query execution when something goes wrong).
// #define INSTRUMENT 1

#ifdef INSTRUMENT
__thread int DBG_depth = 0;
#define DBG(s, args...) do {                                            \
        std::string DBG_s = "";                                         \
        for (int DBG_i = 0; DBG_i < DBG_depth; ++DBG_i) DBG_s += " ";   \
        debugf("%s" s, DBG_s.c_str(), ##args);                          \
    } while (0)
#define INC_DEPTH do { ++DBG_depth; } while (0)
#define DEC_DEPTH do { --DBG_depth; } while (0)
#else // INSTRUMENT
#define DBG(s, args...)
#define INC_DEPTH
#define DEC_DEPTH
#endif // INSTRUMENT

bool term_t::is_deterministic() const {
    bool b = is_deterministic_impl();
    // DBG("%s det: %d\n", name(), b);
    return b;
}

const Term *term_t::get_src() const {
    return src;
}

val_t *term_t::eval() {
    // This is basically a hook for unit tests to change things mid-query
    DEBUG_ONLY_CODE(env->do_eval_callback());
    DBG("EVALUATING %s (%d):\n", name(), is_deterministic());
    env->throw_if_interruptor_pulsed();
    INC_DEPTH;

    try {
        try {
            val_t *ret = eval_impl();
            DEC_DEPTH;
            DBG("%s returned %s\n", name(), ret->print().c_str());
            return ret;
        } catch (const datum_exc_t &e) {
            DEC_DEPTH;
            DBG("%s THREW\n", name());
            rfail("%s", e.what());
        }
    } catch (...) {
        DEC_DEPTH;
        DBG("%s THREW OUTER\n", name());
        throw;
    }
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
val_t *term_t::new_val(datum_stream_t *s, table_t *d) {
    return env->new_val(d, s, this);
}
val_t *term_t::new_val(uuid_u db) { return env->new_val(db, this); }
val_t *term_t::new_val(table_t *t) { return env->new_val(t, this); }
val_t *term_t::new_val(func_t *f) { return env->new_val(f, this); }
val_t *term_t::new_val_bool(bool b) {
    return new_val(new datum_t(datum_t::R_BOOL, b));
}

} //namespace ql
