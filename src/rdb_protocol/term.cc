// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/term.hpp"

#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/stream_cache.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/validate.hpp"

#include "rdb_protocol/terms/terms.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "thread_local.hpp"
#include "protob/protob.hpp"

namespace ql {

counted_t<term_t> compile_term(compile_env_t *env, protob_t<const Term> t) {
    switch (t->type()) {
    case Term::DATUM:              return make_datum_term(t);
    case Term::MAKE_ARRAY:         return make_make_array_term(env, t);
    case Term::MAKE_OBJ:           return make_make_obj_term(env, t);
    case Term::VAR:                return make_var_term(env, t);
    case Term::JAVASCRIPT:         return make_javascript_term(env, t);
    case Term::WGET:               return make_wget_term(env, t);
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
    case Term::GET_FIELD:          return make_get_field_term(env, t);
    case Term::INDEXES_OF:         return make_indexes_of_term(env, t);
    case Term::KEYS:               return make_keys_term(env, t);
    case Term::OBJECT:             return make_object_term(env, t);
    case Term::HAS_FIELDS:         return make_has_fields_term(env, t);
    case Term::WITH_FIELDS:        return make_with_fields_term(env, t);
    case Term::PLUCK:              return make_pluck_term(env, t);
    case Term::WITHOUT:            return make_without_term(env, t);
    case Term::MERGE:              return make_merge_term(env, t);
    case Term::LITERAL:            return make_literal_term(env, t);
    case Term::BETWEEN:            return make_between_term(env, t);
    case Term::REDUCE:             return make_reduce_term(env, t);
    case Term::MAP:                return make_map_term(env, t);
    case Term::FILTER:             return make_filter_term(env, t);
    case Term::CONCATMAP:          return make_concatmap_term(env, t);
    case Term::GROUP:              return make_group_term(env, t);
    case Term::ORDERBY:            return make_orderby_term(env, t);
    case Term::DISTINCT:           return make_distinct_term(env, t);
    case Term::COUNT:              return make_count_term(env, t);
    case Term::SUM:                return make_sum_term(env, t);
    case Term::AVG:                return make_avg_term(env, t);
    case Term::MIN:                return make_min_term(env, t);
    case Term::MAX:                return make_max_term(env, t);
    case Term::UNION:              return make_union_term(env, t);
    case Term::NTH:                return make_nth_term(env, t);
    case Term::LIMIT:              return make_limit_term(env, t);
    case Term::SKIP:               return make_skip_term(env, t);
    case Term::INNER_JOIN:         return make_inner_join_term(env, t);
    case Term::OUTER_JOIN:         return make_outer_join_term(env, t);
    case Term::EQ_JOIN:            return make_eq_join_term(env, t);
    case Term::ZIP:                return make_zip_term(env, t);
    case Term::INSERT_AT:          return make_insert_at_term(env, t);
    case Term::DELETE_AT:          return make_delete_at_term(env, t);
    case Term::CHANGE_AT:          return make_change_at_term(env, t);
    case Term::SPLICE_AT:          return make_splice_at_term(env, t);
    case Term::COERCE_TO:          return make_coerce_term(env, t);
    case Term::UNGROUP:            return make_ungroup_term(env, t);
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
    case Term::SYNC:               return make_sync_term(env, t);
    case Term::INDEX_CREATE:       return make_sindex_create_term(env, t);
    case Term::INDEX_DROP:         return make_sindex_drop_term(env, t);
    case Term::INDEX_LIST:         return make_sindex_list_term(env, t);
    case Term::INDEX_STATUS:       return make_sindex_status_term(env, t);
    case Term::INDEX_WAIT:         return make_sindex_wait_term(env, t);
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
    case Term::SPLIT:              return make_split_term(env, t);
    case Term::UPCASE:             return make_upcase_term(env, t);
    case Term::DOWNCASE:           return make_downcase_term(env, t);
    case Term::SAMPLE:             return make_sample_term(env, t);
    case Term::IS_EMPTY:           return make_is_empty_term(env, t);
    case Term::DEFAULT:            return make_default_term(env, t);
    case Term::JSON:               return make_json_term(env, t);
    case Term::ISO8601:            return make_iso8601_term(env, t);
    case Term::TO_ISO8601:         return make_to_iso8601_term(env, t);
    case Term::EPOCH_TIME:         return make_epoch_time_term(env, t);
    case Term::TO_EPOCH_TIME:      return make_to_epoch_time_term(env, t);
    case Term::NOW:                return make_now_term(env, t);
    case Term::IN_TIMEZONE:        return make_in_timezone_term(env, t);
    case Term::DURING:             return make_during_term(env, t);
    case Term::DATE:               return make_date_term(env, t);
    case Term::TIME_OF_DAY:        return make_time_of_day_term(env, t);
    case Term::TIMEZONE:           return make_timezone_term(env, t);
    case Term::TIME:               return make_time_term(env, t);

    case Term::YEAR:               return make_portion_term(env, t, pseudo::YEAR);
    case Term::MONTH:              return make_portion_term(env, t, pseudo::MONTH);
    case Term::DAY:                return make_portion_term(env, t, pseudo::DAY);
    case Term::DAY_OF_WEEK:        return make_portion_term(env, t,
                                                            pseudo::DAY_OF_WEEK);
    case Term::DAY_OF_YEAR:        return make_portion_term(env, t,
                                                            pseudo::DAY_OF_YEAR);
    case Term::HOURS:              return make_portion_term(env, t, pseudo::HOURS);
    case Term::MINUTES:            return make_portion_term(env, t, pseudo::MINUTES);
    case Term::SECONDS:            return make_portion_term(env, t, pseudo::SECONDS);

    case Term::MONDAY:             return make_constant_term(env, t, 1, "monday");
    case Term::TUESDAY:            return make_constant_term(env, t, 2, "tuesday");
    case Term::WEDNESDAY:          return make_constant_term(env, t, 3, "wednesday");
    case Term::THURSDAY:           return make_constant_term(env, t, 4, "thursday");
    case Term::FRIDAY:             return make_constant_term(env, t, 5, "friday");
    case Term::SATURDAY:           return make_constant_term(env, t, 6, "saturday");
    case Term::SUNDAY:             return make_constant_term(env, t, 7, "sunday");
    case Term::JANUARY:            return make_constant_term(env, t, 1, "january");
    case Term::FEBRUARY:           return make_constant_term(env, t, 2, "february");
    case Term::MARCH:              return make_constant_term(env, t, 3, "march");
    case Term::APRIL:              return make_constant_term(env, t, 4, "april");
    case Term::MAY:                return make_constant_term(env, t, 5, "may");
    case Term::JUNE:               return make_constant_term(env, t, 6, "june");
    case Term::JULY:               return make_constant_term(env, t, 7, "july");
    case Term::AUGUST:             return make_constant_term(env, t, 8, "august");
    case Term::SEPTEMBER:          return make_constant_term(env, t, 9, "september");
    case Term::OCTOBER:            return make_constant_term(env, t, 10, "october");
    case Term::NOVEMBER:           return make_constant_term(env, t, 11, "november");
    case Term::DECEMBER:           return make_constant_term(env, t, 12, "december");
    default: unreachable();
    }
    unreachable();
}

void run(protob_t<Query> q,
         rdb_protocol_t::context_t *ctx,
         signal_t *interruptor,
         Response *res,
         stream_cache2_t *stream_cache2) {
    try {
        validate_pb(*q);
    } catch (const base_exc_t &e) {
        fill_error(res, Response::CLIENT_ERROR, e.what(), backtrace_t());
        return;
    }
#ifdef INSTRUMENT
    debugf("Query: %s\n", q->DebugString().c_str());
#endif // INSTRUMENT

    int64_t token = q->token();
    use_json_t use_json = q->accepts_r_json() ? use_json_t::YES : use_json_t::NO;

    switch (q->type()) {
    case Query_QueryType_START: {
        threadnum_t th = get_thread_id();
        scoped_ptr_t<ql::env_t> env(
            new ql::env_t(
                ctx->extproc_pool, ctx->ns_repo,
                ctx->cross_thread_namespace_watchables[th.threadnum]->get_watchable(),
                ctx->cross_thread_database_watchables[th.threadnum]->get_watchable(),
                ctx->cluster_metadata, ctx->directory_read_manager,
                interruptor, ctx->machine_id, q));

        counted_t<term_t> root_term;
        try {
            Term *t = q->mutable_query();
            compile_env_t compile_env((var_visibility_t()));
            root_term = compile_term(&compile_env, q.make_child(t));
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
            scope_env_t scope_env(env.get(), var_scope_t());
            counted_t<val_t> val = root_term->eval(&scope_env);
            if (val->get_type().is_convertible(val_t::type_t::DATUM)) {
                res->set_type(Response_ResponseType_SUCCESS_ATOM);
                counted_t<const datum_t> d = val->as_datum();
                d->write_to_protobuf(res->add_response(), use_json);
                if (env->trace.has()) {
                    env->trace->as_datum()->write_to_protobuf(
                        res->mutable_profile(), use_json);
                }
            } else if (counted_t<grouped_data_t> gd
                       = val->maybe_as_promiscuous_grouped_data(scope_env.env)) {
                res->set_type(Response::SUCCESS_ATOM);
                datum_t d(std::move(*gd));
                d.write_to_protobuf(res->add_response(), use_json);
                if (env->trace.has()) {
                    env->trace->as_datum()->write_to_protobuf(
                        res->mutable_profile(), use_json);
                }
            } else if (val->get_type().is_convertible(val_t::type_t::SEQUENCE)) {
                counted_t<datum_stream_t> seq = val->as_seq(env.get());
                if (counted_t<const datum_t> arr = seq->as_array(env.get())) {
                    res->set_type(Response_ResponseType_SUCCESS_ATOM);
                    arr->write_to_protobuf(res->add_response(), use_json);
                    if (env->trace.has()) {
                        env->trace->as_datum()->write_to_protobuf(
                            res->mutable_profile(), use_json);
                    }
                } else {
                    stream_cache2->insert(token, use_json, std::move(env), seq);
                    bool b = stream_cache2->serve(token, res, interruptor);
                    r_sanity_check(b);
                }
            } else {
                rfail_toplevel(base_exc_t::GENERIC,
                               "Query result must be of type "
                               "DATUM, GROUPED_DATA, or STREAM (got %s).",
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
            bool b = stream_cache2->serve(token, res, interruptor);
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
            res->set_type(Response::SUCCESS_SEQUENCE);
        } catch (const exc_t &e) {
            fill_error(res, Response::CLIENT_ERROR, e.what(), e.backtrace());
            return;
        }
    } break;
    case Query_QueryType_NOREPLY_WAIT: {
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

        // NOREPLY_WAIT is just a no-op.
        // This works because we only evaluate one Query at a time
        // on the connection level. Once we get to the NOREPLY_WAIT Query
        // we know that all previous Queries have completed processing.

        // Send back a WAIT_COMPLETE response.
        res->set_type(Response_ResponseType_WAIT_COMPLETE);
    } break;
    default: unreachable();
    }
}

term_t::term_t(protob_t<const Term> _src)
    : pb_rcheckable_t(get_backtrace(_src)), src(_src) { }
term_t::~term_t() { }

// Uncomment the define to enable instrumentation (you'll be able to see where
// you are in query execution when something goes wrong).
// #define INSTRUMENT 1

#ifdef INSTRUMENT
TLS_with_init(int, DBG_depth, 0);
#define DBG(s, args...) do {                                            \
        std::string DBG_s = "";                                         \
        for (int DBG_i = 0; DBG_i < TLS_get_DBG_depth(); ++DBG_i) DBG_s += " ";   \
        debugf("%s" s, DBG_s.c_str(), ##args);                          \
    } while (0)
#define INC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()+1); } while (0)
#define DEC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()-1); } while (0)
#else // INSTRUMENT
#define DBG(s, args...)
#define INC_DEPTH
#define DEC_DEPTH
#endif // INSTRUMENT

protob_t<const Term> term_t::get_src() const {
    return src;
}

void term_t::prop_bt(Term *t) const {
    propagate_backtrace(t, &get_src()->GetExtension(ql2::extension::backtrace));
}

counted_t<val_t> term_t::eval(scope_env_t *env, eval_flags_t eval_flags) {
    // This is basically a hook for unit tests to change things mid-query
    profile::starter_t starter(strprintf("Evaluating %s.", name()), env->env->trace);
    DEBUG_ONLY_CODE(env->env->do_eval_callback());
    DBG("EVALUATING %s (%d):\n", name(), is_deterministic());
    env->env->throw_if_interruptor_pulsed();
    env->env->maybe_yield();
    INC_DEPTH;

    try {
        try {
            counted_t<val_t> ret = term_eval(env, eval_flags);
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
    return make_counted<val_t>(d, backtrace());
}
counted_t<val_t> term_t::new_val(counted_t<const datum_t> d, counted_t<table_t> t) {
    return make_counted<val_t>(d, t, backtrace());
}

counted_t<val_t> term_t::new_val(counted_t<const datum_t> d,
                                 counted_t<const datum_t> orig_key,
                                 counted_t<table_t> t) {
    return make_counted<val_t>(d, orig_key, t, backtrace());
}

counted_t<val_t> term_t::new_val(env_t *env, counted_t<datum_stream_t> s) {
    return make_counted<val_t>(env, s, backtrace());
}
counted_t<val_t> term_t::new_val(counted_t<datum_stream_t> s, counted_t<table_t> d) {
    return make_counted<val_t>(d, s, backtrace());
}
counted_t<val_t> term_t::new_val(counted_t<const db_t> db) {
    return make_counted<val_t>(db, backtrace());
}
counted_t<val_t> term_t::new_val(counted_t<table_t> t) {
    return make_counted<val_t>(t, backtrace());
}
counted_t<val_t> term_t::new_val(counted_t<func_t> f) {
    return make_counted<val_t>(f, backtrace());
}
counted_t<val_t> term_t::new_val_bool(bool b) {
    return new_val(make_counted<const datum_t>(datum_t::R_BOOL, b));
}

} // namespace ql
