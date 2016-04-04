// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/term.hpp"

#include "arch/address.hpp"
#include "clustering/administration/jobs/report.hpp"
#include "concurrency/cross_thread_watchable.hpp"
#include "rdb_protocol/rdb_backtrace.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/query_cache.hpp"
#include "rdb_protocol/response.hpp"
#include "rdb_protocol/term_walker.hpp"
#include "rdb_protocol/terms/terms.hpp"
#include "thread_local.hpp"

namespace ql {

// The minimum amount of stack space we require to be available on a coroutine
// before attempting to compile or evaluate a term.
const size_t MIN_COMPILE_STACK_SPACE = 16 * KILOBYTE;
const size_t MIN_EVAL_STACK_SPACE = 16 * KILOBYTE;

counted_t<const term_t> compile_on_current_stack(
        compile_env_t *env,
        const raw_term_t &t) {
    switch (t.type()) {
    case Term::DATUM:              return make_datum_term(t);
    case Term::MAKE_ARRAY:         return make_make_array_term(env, t);
    case Term::MAKE_OBJ:           return make_make_obj_term(env, t);
    case Term::BINARY:             return make_binary_term(env, t);
    case Term::VAR:                return make_var_term(env, t);
    case Term::JAVASCRIPT:         return make_javascript_term(env, t);
    case Term::HTTP:               return make_http_term(env, t);
    case Term::ERROR:              return make_error_term(env, t);
    case Term::IMPLICIT_VAR:       return make_implicit_var_term(env, t);
    case Term::RANDOM:             return make_random_term(env, t);
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
    case Term::OFFSETS_OF:         return make_offsets_of_term(env, t);
    case Term::KEYS:               return make_keys_term(env, t);
    case Term::VALUES:             return make_values_term(env, t);
    case Term::OBJECT:             return make_object_term(env, t);
    case Term::HAS_FIELDS:         return make_has_fields_term(env, t);
    case Term::WITH_FIELDS:        return make_with_fields_term(env, t);
    case Term::PLUCK:              return make_pluck_term(env, t);
    case Term::WITHOUT:            return make_without_term(env, t);
    case Term::MERGE:              return make_merge_term(env, t);
    case Term::LITERAL:            return make_literal_term(env, t);
    case Term::ARGS:               return make_args_term(env, t);
    case Term::BETWEEN_DEPRECATED: return make_between_deprecated_term(env, t);
    case Term::BETWEEN:            return make_between_term(env, t);
    case Term::CHANGES:            return make_changes_term(env, t);
    case Term::REDUCE:             return make_reduce_term(env, t);
    case Term::MAP:                return make_map_term(env, t);
    case Term::FOLD:               return make_fold_term(env, t);
    case Term::FILTER:             return make_filter_term(env, t);
    case Term::CONCAT_MAP:         return make_concatmap_term(env, t);
    case Term::GROUP:              return make_group_term(env, t);
    case Term::ORDER_BY:           return make_orderby_term(env, t);
    case Term::DISTINCT:           return make_distinct_term(env, t);
    case Term::COUNT:              return make_count_term(env, t);
    case Term::SUM:                return make_sum_term(env, t);
    case Term::AVG:                return make_avg_term(env, t);
    case Term::MIN:                return make_min_term(env, t);
    case Term::MAX:                return make_max_term(env, t);
    case Term::UNION:              return make_union_term(env, t);
    case Term::NTH:                return make_nth_term(env, t);
    case Term::BRACKET:            return make_bracket_term(env, t);
    case Term::LIMIT:              return make_limit_term(env, t);
    case Term::SKIP:               return make_skip_term(env, t);
    case Term::INNER_JOIN:         return make_inner_join_term(env, t);
    case Term::OUTER_JOIN:         return make_outer_join_term(env, t);
    case Term::EQ_JOIN:            return make_eq_join_term(env, t);
    case Term::ZIP:                return make_zip_term(env, t);
    case Term::RANGE:              return make_range_term(env, t);
    case Term::INSERT_AT:          return make_insert_at_term(env, t);
    case Term::DELETE_AT:          return make_delete_at_term(env, t);
    case Term::CHANGE_AT:          return make_change_at_term(env, t);
    case Term::SPLICE_AT:          return make_splice_at_term(env, t);
    case Term::COERCE_TO:          return make_coerce_term(env, t);
    case Term::UNGROUP:            return make_ungroup_term(env, t);
    case Term::TYPE_OF:            return make_typeof_term(env, t);
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
    case Term::CONFIG:             return make_config_term(env, t);
    case Term::STATUS:             return make_status_term(env, t);
    case Term::WAIT:               return make_wait_term(env, t);
    case Term::RECONFIGURE:        return make_reconfigure_term(env, t);
    case Term::REBALANCE:          return make_rebalance_term(env, t);
    case Term::SYNC:               return make_sync_term(env, t);
    case Term::GRANT:              return make_grant_term(env, t);
    case Term::INDEX_CREATE:       return make_sindex_create_term(env, t);
    case Term::INDEX_DROP:         return make_sindex_drop_term(env, t);
    case Term::INDEX_LIST:         return make_sindex_list_term(env, t);
    case Term::INDEX_STATUS:       return make_sindex_status_term(env, t);
    case Term::INDEX_WAIT:         return make_sindex_wait_term(env, t);
    case Term::INDEX_RENAME:       return make_sindex_rename_term(env, t);
    case Term::FUNCALL:            return make_funcall_term(env, t);
    case Term::BRANCH:             return make_branch_term(env, t);
    case Term::OR:                 return make_or_term(env, t);
    case Term::AND:                return make_and_term(env, t);
    case Term::FOR_EACH:           return make_foreach_term(env, t);
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
    case Term::TO_JSON_STRING:     return make_to_json_string_term(env, t);
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
    case Term::GEOJSON:            return make_geojson_term(env, t);
    case Term::TO_GEOJSON:         return make_to_geojson_term(env, t);
    case Term::POINT:              return make_point_term(env, t);
    case Term::LINE:               return make_line_term(env, t);
    case Term::POLYGON:            return make_polygon_term(env, t);
    case Term::DISTANCE:           return make_distance_term(env, t);
    case Term::INTERSECTS:         return make_intersects_term(env, t);
    case Term::INCLUDES:           return make_includes_term(env, t);
    case Term::CIRCLE:             return make_circle_term(env, t);
    case Term::GET_INTERSECTING:   return make_get_intersecting_term(env, t);
    case Term::FILL:               return make_fill_term(env, t);
    case Term::GET_NEAREST:        return make_get_nearest_term(env, t);
    case Term::UUID:               return make_uuid_term(env, t);
    case Term::POLYGON_SUB:        return make_polygon_sub_term(env, t);
    case Term::MINVAL:             return make_minval_term(env, t);
    case Term::MAXVAL:             return make_maxval_term(env, t);
    case Term::FLOOR:              return make_floor_term(env, t);
    case Term::CEIL:               return make_ceil_term(env, t);
    case Term::ROUND:              return make_round_term(env, t);
    default: unreachable();
    }
    unreachable();
}

counted_t<const term_t> compile_term(compile_env_t *env, const raw_term_t &t) {
    return call_with_enough_stack<counted_t<const term_t> >([&]() {
            return compile_on_current_stack(env, std::move(t));
        }, MIN_COMPILE_STACK_SPACE);
}

runtime_term_t::runtime_term_t(backtrace_id_t bt)
    : bt_rcheckable_t(bt) { }

runtime_term_t::~runtime_term_t() { }

term_t::term_t(const raw_term_t &_src)
    : runtime_term_t(_src.bt()),
      src(_src) { }

term_t::~term_t() { }

// Uncomment the define to enable instrumentation (you'll be able to see where
// you are in query execution when something goes wrong).
// #define INSTRUMENT 1

#ifdef INSTRUMENT
TLS_with_init(int, DBG_depth, 0);
#define DBG(s, ...) do {                                                \
        std::string DBG_s = "";                                         \
        for (int DBG_i = 0; DBG_i < TLS_get_DBG_depth(); ++DBG_i) DBG_s += " ";   \
        debugf("%s" s, DBG_s.c_str(), ##__VA_ARGS__);                   \
    } while (0)
#define INC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()+1); } while (0)
#define DEC_DEPTH do { TLS_set_DBG_depth(TLS_get_DBG_depth()-1); } while (0)
#else // INSTRUMENT
#define DBG(s, ...)
#define INC_DEPTH
#define DEC_DEPTH
#endif // INSTRUMENT

const raw_term_t &term_t::get_src() const {
    return src;
}

scoped_ptr_t<val_t> runtime_term_t::eval_on_current_stack(
        scope_env_t *env,
        eval_flags_t eval_flags) const {
        PROFILE_STARTER_IF_ENABLED(
        env->env->profile() == profile_bool_t::PROFILE,
        strprintf("Evaluating %s.", name()),
        env->env->trace);
    // This is basically a hook for unit tests to change things mid-query
    env->env->do_eval_callback();
    DBG("EVALUATING %s (%d):\n", name(), is_deterministic());
    if (env->env->interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    env->env->maybe_yield();
    INC_DEPTH;

#ifdef INSTRUMENT
    try {
#endif // INSTRUMENT
        try {
            scoped_ptr_t<val_t> ret = term_eval(env, eval_flags);
            DEC_DEPTH;
            DBG("%s returned %s\n", name(), ret->print().c_str());
            return ret;
        } catch (const datum_exc_t &e) {
            DEC_DEPTH;
            DBG("%s THREW\n", name());
            rfail(e.get_type(), "%s", e.what());
        }
#ifdef INSTRUMENT
    } catch (...) {
        DEC_DEPTH;
        DBG("%s THREW OUTER\n", name());
        throw;
    }
#endif // INSTRUMENT
}

scoped_ptr_t<val_t> runtime_term_t::eval(
        scope_env_t *env,
        eval_flags_t eval_flags) const {
    return call_with_enough_stack<scoped_ptr_t<val_t> >([&]() {
            return eval_on_current_stack(env, std::move(eval_flags));
        }, MIN_EVAL_STACK_SPACE);
}

} // namespace ql
