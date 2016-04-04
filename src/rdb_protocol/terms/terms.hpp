// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TERMS_TERMS_HPP_
#define RDB_PROTOCOL_TERMS_TERMS_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/pseudo_time.hpp"

namespace ql {
class compile_env_t;
class term_t;
class raw_term_t;
class configured_limits_t;

// arith.cc
counted_t<term_t> make_arith_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_mod_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_floor_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_ceil_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_round_term(
    compile_env_t *env, const raw_term_t &term);

// random.cc
counted_t<term_t> make_sample_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_random_term(
    compile_env_t *env, const raw_term_t &term);

// arr.cc
counted_t<term_t> make_args_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_contains_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_append_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_prepend_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_nth_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_bracket_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_is_empty_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_slice_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_limit_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_set_insert_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_set_union_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_set_intersection_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_set_difference_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_insert_at_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_delete_at_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_change_at_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_splice_at_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_offsets_of_term(
    compile_env_t *env, const raw_term_t &term);

// control.cc
counted_t<term_t> make_and_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_or_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_branch_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_funcall_term(
    compile_env_t *env, const raw_term_t &term);

// datum_terms.cc
counted_t<term_t> make_datum_term(
    const raw_term_t &term);
counted_t<term_t> make_constant_term(
    compile_env_t *env, const raw_term_t &term,
    double constant, const char *name);
counted_t<term_t> make_make_array_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_make_obj_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_binary_term(
    compile_env_t *env, const raw_term_t &term);

// db_table.cc
counted_t<term_t> make_db_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_table_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_get_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_get_all_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_db_create_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_db_drop_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_db_list_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_table_create_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_table_drop_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_table_list_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_config_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_status_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_wait_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_reconfigure_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_rebalance_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sync_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_grant_term(
    compile_env_t *env, const raw_term_t &term);

// error.cc
counted_t<term_t> make_error_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_default_term(
    compile_env_t *env, const raw_term_t &term);

// geo.cc
counted_t<term_t> make_geojson_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_to_geojson_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_point_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_line_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_polygon_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_intersects_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_includes_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_distance_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_circle_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_get_intersecting_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_fill_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_get_nearest_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_polygon_sub_term(
    compile_env_t *env, const raw_term_t &term);

// js.cc
counted_t<term_t> make_javascript_term(
    compile_env_t *env, const raw_term_t &term);

// uuid.cc
counted_t<term_t> make_uuid_term(
    compile_env_t *env, const raw_term_t &term);

// http.cc
counted_t<term_t> make_http_term(
    compile_env_t *env, const raw_term_t &term);

// json.cc
counted_t<term_t> make_json_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_to_json_string_term(
    compile_env_t *env, const raw_term_t &term);

// match.cc
counted_t<term_t> make_match_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_split_term(
    compile_env_t *env, const raw_term_t &term);

// case.cc
counted_t<term_t> make_upcase_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_downcase_term(
    compile_env_t *env, const raw_term_t &term);

// obj.cc
counted_t<term_t> make_keys_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_values_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_object_term(
    compile_env_t *env, const raw_term_t &term);

// obj_or_seq.cc
counted_t<term_t> make_pluck_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_without_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_literal_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_merge_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_has_fields_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_get_field_term(
    compile_env_t *env, const raw_term_t &term);

// pred.cc
counted_t<term_t> make_predicate_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_not_term(
    compile_env_t *env, const raw_term_t &term);

// rewrites.cc
counted_t<term_t> make_skip_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_inner_join_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_outer_join_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_eq_join_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_update_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_delete_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_difference_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_with_fields_term(
    compile_env_t *env, const raw_term_t &term);

// seq.cc
counted_t<term_t> make_minval_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_maxval_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_between_deprecated_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_between_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_changes_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_reduce_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_map_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_fold_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_filter_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_concatmap_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_group_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_count_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sum_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_avg_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_min_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_max_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_union_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_zip_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_range_term(
    compile_env_t *env, const raw_term_t &term);

// sindex.cc
counted_t<term_t> make_sindex_create_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sindex_drop_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sindex_list_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sindex_status_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sindex_wait_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_sindex_rename_term(
    compile_env_t *env, const raw_term_t &term);

// sort.cc
counted_t<term_t> make_orderby_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_distinct_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_asc_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_desc_term(
    compile_env_t *env, const raw_term_t &term);

// time.cc
counted_t<term_t> make_iso8601_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_to_iso8601_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_epoch_time_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_to_epoch_time_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_now_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_in_timezone_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_during_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_date_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_time_of_day_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_timezone_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_time_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_portion_term(
    compile_env_t *env, const raw_term_t &term,
    pseudo::time_component_t component);

// type_manip.cc
counted_t<term_t> make_coerce_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_ungroup_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_typeof_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_info_term(
    compile_env_t *env, const raw_term_t &term);

// var.cc
counted_t<term_t> make_var_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_implicit_var_term(
    compile_env_t *env, const raw_term_t &term);

// writes.cc
counted_t<term_t> make_insert_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_replace_term(
    compile_env_t *env, const raw_term_t &term);
counted_t<term_t> make_foreach_term(
    compile_env_t *env, const raw_term_t &term);

} // namespace ql

#endif  // RDB_PROTOCOL_TERMS_TERMS_HPP_
