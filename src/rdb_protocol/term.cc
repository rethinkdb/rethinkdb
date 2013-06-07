// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "rdb_protocol/term.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/pb_utils.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/validate.hpp"

#include "rdb_protocol/terms/terms.hpp"

#pragma GCC diagnostic ignored "-Wshadow"

namespace ql {

counted_t<term_t> compile_term(env_t *env, protob_t<const Term> t) {
    switch (t->type()) {
    case Term::DATUM:              return make_datum_term(env, t);
    case Term::MAKE_ARRAY:         return make_make_array_term(env, t);
    case Term::MAKE_OBJ:           return make_make_obj_term(env, t);
    case Term::VAR:                return make_var_term(env, t);
    case Term::JAVASCRIPT:         return make_javascript_term(env, t);
    case Term::ERROR:              return make_error_term(env, t);
    case Term::IMPLICIT_VAR:       return make_implicit_var_term(env, t);
    case Term::DB:                 return make_db_term(env, t);
    case Term::TABLE:              return make_table_term(env, t);
    case Term::GET:                return make_get_term(env, t);
    case Term::GET_ALL:            return make_get_all_term(env, t);
    case Term::EQ:                 // fallthru
    case Term::NE:                 // fallthru
    case Term::LT:                 // fallthru
    case Term::LE:                 // fallthru
    case Term::GT:                 // fallthru
    case Term::GE:                 return make_predicate_term(env, t);
    case Term::NOT:                return make_not_term(env, t);
    case Term::ADD:                // fallthru
    case Term::SUB:                // fallthru
    case Term::MUL:                // fallthru
    case Term::DIV:                return make_arith_term(env, t);
    case Term::MOD:                return make_mod_term(env, t);
    case Term::CONTAINS:           return make_contains_term(env, t);
    case Term::APPEND:             return make_append_term(env, t);
    case Term::PREPEND:            return make_prepend_term(env, t);
    case Term::DIFFERENCE:         return make_difference_term(env, t);
    case Term::SET_INSERT:         return make_set_insert_term(env, t);
    case Term::SET_INTERSECTION:   return make_set_intersection_term(env, t);
    case Term::SET_UNION:          return make_set_union_term(env, t);
    case Term::SET_DIFFERENCE:     return make_set_difference_term(env, t);
    case Term::SLICE:              return make_slice_term(env, t);
    case Term::GETATTR:            return make_getattr_term(env, t);
    case Term::INDEXES_OF:         return make_indexes_of_term(env, t);
    case Term::KEYS:               return make_keys_term(env, t);
    case Term::HAS_FIELDS:         return make_has_fields_term(env, t);
    case Term::WITH_FIELDS:        return make_with_fields_term(env, t);
    case Term::PLUCK:              return make_pluck_term(env, t);
    case Term::WITHOUT:            return make_without_term(env, t);
    case Term::MERGE:              return make_merge_term(env, t);
    case Term::BETWEEN:            return make_between_term(env, t);
    case Term::REDUCE:             return make_reduce_term(env, t);
    case Term::MAP:                return make_map_term(env, t);
    case Term::FILTER:             return make_filter_term(env, t);
    case Term::CONCATMAP:          return make_concatmap_term(env, t);
    case Term::ORDERBY:            return make_orderby_term(env, t);
    case Term::DISTINCT:           return make_distinct_term(env, t);
    case Term::COUNT:              return make_count_term(env, t);
    case Term::UNION:              return make_union_term(env, t);
    case Term::NTH:                return make_nth_term(env, t);
    case Term::GROUPED_MAP_REDUCE: return make_gmr_term(env, t);
    case Term::LIMIT:              return make_limit_term(env, t);
    case Term::SKIP:               return make_skip_term(env, t);
    case Term::GROUPBY:            return make_groupby_term(env, t);
    case Term::INNER_JOIN:         return make_inner_join_term(env, t);
    case Term::OUTER_JOIN:         return make_outer_join_term(env, t);
    case Term::EQ_JOIN:            return make_eq_join_term(env, t);
    case Term::ZIP:                return make_zip_term(env, t);
    case Term::INSERT_AT:          return make_insert_at_term(env, t);
    case Term::DELETE_AT:          return make_delete_at_term(env, t);
    case Term::CHANGE_AT:          return make_change_at_term(env, t);
    case Term::SPLICE_AT:          return make_splice_at_term(env, t);
    case Term::COERCE_TO:          return make_coerce_term(env, t);
    case Term::TYPEOF:             return make_typeof_term(env, t);
    case Term::UPDATE:             return make_update_term(env, t);
    case Term::DELETE:             return make_delete_term(env, t);
    case Term::REPLACE:            return make_replace_term(env, t);
    case Term::INSERT:             return make_insert_term(env, t);
    case Term::DB_CREATE:          return make_db_create_term(env, t);
    case Term::DB_DROP:            return make_db_drop_term(env, t);
    case Term::DB_LIST:            return make_db_list_term(env, t);
    case Term::TABLE_CREATE:       return make_table_create_term(env, t);
    case Term::TABLE_DROP:         return make_table_drop_term(env, t);
    case Term::TABLE_LIST:         return make_table_list_term(env, t);
    case Term::INDEX_CREATE:       return make_sindex_create_term(env, t);
    case Term::INDEX_DROP:         return make_sindex_drop_term(env, t);
    case Term::INDEX_LIST:         return make_sindex_list_term(env, t);
    case Term::FUNCALL:            return make_funcall_term(env, t);
    case Term::BRANCH:             return make_branch_term(env, t);
    case Term::ANY:                return make_any_term(env, t);
    case Term::ALL:                return make_all_term(env, t);
    case Term::FOREACH:            return make_foreach_term(env, t);
    case Term::FUNC:               return make_counted<func_term_t>(env, t);
    case Term::ASC:                return make_asc_term(env, t);
    case Term::DESC:               return make_desc_term(env, t);
    case Term::INFO:               return make_info_term(env, t);
    case Term::MATCH:              return make_match_term(env, t);
    case Term::SAMPLE:             return make_sample_term(env, t);
    case Term::IS_EMPTY:           return make_is_empty_term(env, t);
    case Term::DEFAULT:            return make_default_term(env, t);
    default: unreachable();
    }
    unreachable();
}

void run(protob_t<Query> q, scoped_ptr_t<env_t> *env_ptr,
         Response *res, stream_cache2_t *stream_cache2,
         bool *response_needed_out) {
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
        counted_t<term_t> root_term;
        try {
            Term *t = q->mutable_query();
            preprocess_term(t);
            Backtrace *t_bt = t->MutableExtension(ql2::extension::backtrace);


            // We parse out the `noreply` optarg in a special step so that we
            // don't send back an unneeded response in the case where another
            // optional argument throws a compilation error.
            for (int i = 0; i < q->global_optargs_size(); ++i) {
                const Query::AssocPair &ap = q->global_optargs(i);
                if (ap.key() == "noreply") {
                    bool conflict = env->add_optarg(ap.key(), ap.val());
                    r_sanity_check(!conflict);
                    counted_t<val_t> noreply = env->get_optarg("noreply");
                    r_sanity_check(noreply.has());
                    *response_needed_out = !noreply->as_bool();
                    break;
                }
            }

            // Parse global optargs
            for (int i = 0; i < q->global_optargs_size(); ++i) {
                const Query::AssocPair &ap = q->global_optargs(i);
                if (ap.key() != "noreply") {
                    bool conflict = env->add_optarg(ap.key(), ap.val());
                    rcheck_toplevel(
                        !conflict, base_exc_t::GENERIC,
                        strprintf("Duplicate global optarg: %s", ap.key().c_str()));
                }
            }

            protob_t<Term> ewt = make_counted_term();
            Term *const arg = ewt.get();

            N1(DB, NDATUM("test"));

            propagate_backtrace(arg, t_bt); // duplicate toplevel backtrace
            UNUSED bool _b = env->add_optarg("db", *arg);
            //          ^^ UNUSED because user can override this value safely

            // Parse actual query
            root_term = compile_term(env, q.make_child(t));
            // TODO: handle this properly
        } catch (const exc_t &e) {
            fill_error(res, Response::COMPILE_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::COMPILE_ERROR, e.what(), backtrace_t());
            return;
        }

        try {
            rcheck_toplevel(!stream_cache2->contains(token),
                            base_exc_t::GENERIC,
                            strprintf("ERROR: duplicate token %" PRIi64, token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
            return;
        }

        try {
            counted_t<val_t> val = root_term->eval();

            if (!*response_needed_out) {
                // It's fine to just abort here because we don't allow write
                // operations inside of lazy operations, which means the writes
                // will have already occured even if `val` is a sequence that we
                // haven't yet exhuasted.
                return;
            }

            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response_ResponseType_SUCCESS_ATOM);
                counted_t<const datum_t> d = val->as_datum();
                d->write_to_protobuf(res->add_response());
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                stream_cache2->insert(token, env_ptr, val->as_seq());
                bool b = stream_cache2->serve(token, res, env->interruptor);
                r_sanity_check(b);
            } else {
                rfail_toplevel(base_exc_t::GENERIC,
                               "Query result must be of type DATUM or STREAM (got %s).",
                               val->get_type().name());
            }
        } catch (const exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), e.backtrace());
            return;
        } catch (const datum_exc_t &e) {
            fill_error(res, Response::RUNTIME_ERROR, e.what(), backtrace_t());
            return;
        }

    } break;
    case Query_QueryType_CONTINUE: {
        try {
            bool b = stream_cache2->serve(token, res, env->interruptor);
            rcheck_toplevel(b, base_exc_t::GENERIC,
                            strprintf("Token %" PRIi64 " not in stream cache.", token));
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        }
    } break;
    case Query_QueryType_STOP: {
        try {
            rcheck_toplevel(stream_cache2->contains(token), base_exc_t::GENERIC,
                            strprintf("Token %" PRIi64 " not in stream cache.", token));
            stream_cache2->erase(token);
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        }
    } break;
    default: unreachable();
    }
}

term_t::term_t(env_t *_env, protob_t<const Term> _src)
    : pb_rcheckable_t(_src), env(_env), src(_src) {
    guarantee(env != NULL);
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

protob_t<const Term> term_t::get_src() const {
    return src;
}

void term_t::prop_bt(Term *t) const {
    propagate_backtrace(t, &get_src()->GetExtension(ql2::extension::backtrace));
}

counted_t<val_t> term_t::eval() {
    // This is basically a hook for unit tests to change things mid-query
    DEBUG_ONLY_CODE(env->do_eval_callback());
    DBG("EVALUATING %s (%d):\n", name(), is_deterministic());
    env->throw_if_interruptor_pulsed();
    INC_DEPTH;

    try {
        try {
            counted_t<val_t> ret = eval_impl();
            DEC_DEPTH;
            DBG("%s returned %s\n", name(), ret->print().c_str());
            return ret;
        } catch (const datum_exc_t &e) {
            DEC_DEPTH;
            DBG("%s THREW\n", name());
            rfail(e.get_type(), "%s", e.what());
        }
    } catch (...) {
        DEC_DEPTH;
        DBG("%s THREW OUTER\n", name());
        throw;
    }
}

counted_t<val_t> term_t::new_val(counted_t<const datum_t> d) {
    return make_counted<val_t>(d, this);
}
counted_t<val_t> term_t::new_val(counted_t<const datum_t> d, counted_t<table_t> t) {
    return make_counted<val_t>(d, t, this);
}

counted_t<val_t> term_t::new_val(counted_t<datum_stream_t> s) {
    return make_counted<val_t>(s, this);
}
counted_t<val_t> term_t::new_val(counted_t<datum_stream_t> s, counted_t<table_t> d) {
    return make_counted<val_t>(d, s, this);
}
counted_t<val_t> term_t::new_val(counted_t<const db_t> db) {
    return make_counted<val_t>(db, this);
}
counted_t<val_t> term_t::new_val(counted_t<table_t> t) {
    return make_counted<val_t>(t, this);
}
counted_t<val_t> term_t::new_val(counted_t<func_t> f) {
    return make_counted<val_t>(f, this);
}
counted_t<val_t> term_t::new_val_bool(bool b) {
    return new_val(make_counted<const datum_t>(datum_t::R_BOOL, b));
}

} //namespace ql
