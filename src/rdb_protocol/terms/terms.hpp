#ifndef RDB_PROTOCOL_TERMS_TERMS_HPP_
#define RDB_PROTOCOL_TERMS_TERMS_HPP_

#include "containers/counted.hpp"
#include "rdb_protocol/counted_term.hpp"
#include "rdb_protocol/pseudo_time.hpp"
#include "rdb_protocol/term.hpp"

namespace ql {
class env_t;
class term_t;

// arith.cc
counted_t<term_t> make_arith_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_mod_term(env_t *env, const protob_t<const Term> &term);

// random.cc
counted_t<term_t> make_sample_term(env_t *env, const protob_t<const Term> &term);

// arr.cc
counted_t<term_t> make_contains_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_append_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_prepend_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_nth_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_is_empty_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_slice_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_limit_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_set_insert_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_set_union_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_set_intersection_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_set_difference_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_insert_at_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_delete_at_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_change_at_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_splice_at_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_indexes_of_term(env_t *env, const protob_t<const Term> &term);

// control.cc
counted_t<term_t> make_all_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_any_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_branch_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_funcall_term(env_t *env, const protob_t<const Term> &term);

// datum_terms.cc
counted_t<term_t> make_datum_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_constant_term(env_t *env, const protob_t<const Term> &term,
                                     double constant, const char *name);
counted_t<term_t> make_make_array_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_make_obj_term(env_t *env, const protob_t<const Term> &term,
                                     eval_flags_t flags);

// db_table.cc
counted_t<term_t> make_db_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_table_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_get_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_get_all_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_db_create_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_db_drop_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_db_list_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_table_create_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_table_drop_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_table_list_term(env_t *env, const protob_t<const Term> &term);

// error.cc
counted_t<term_t> make_error_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_default_term(env_t *env, const protob_t<const Term> &term);

// gmr.cc
counted_t<term_t> make_gmr_term(env_t *env, const protob_t<const Term> &term);

// js.cc
counted_t<term_t> make_javascript_term(env_t *env, const protob_t<const Term> &term);

// json.cc
counted_t<term_t> make_json_term(env_t *env, const protob_t<const Term> &term);

// match.cc
counted_t<term_t> make_match_term(env_t *env, const protob_t<const Term> &term);

// obj.cc
counted_t<term_t> make_keys_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_get_field_term(env_t *env, const protob_t<const Term> &term);

// obj_or_seq.cc
counted_t<term_t> make_pluck_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_without_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_literal_term(env_t *env, const protob_t<const Term> &term, eval_flags_t flags);
counted_t<term_t> make_merge_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_has_fields_term(env_t *env, const protob_t<const Term> &term);

// pred.cc
counted_t<term_t> make_predicate_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_not_term(env_t *env, const protob_t<const Term> &term);

// rewrites.cc
counted_t<term_t> make_skip_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_groupby_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_inner_join_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_outer_join_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_eq_join_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_update_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_delete_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_difference_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_with_fields_term(env_t *env, const protob_t<const Term> &term);

// seq.cc
counted_t<term_t> make_between_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_reduce_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_map_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_filter_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_concatmap_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_count_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_union_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_zip_term(env_t *env, const protob_t<const Term> &term);

// sindex.cc
counted_t<term_t> make_sindex_create_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_sindex_drop_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_sindex_list_term(env_t *env, const protob_t<const Term> &term);

// sort.cc
counted_t<term_t> make_orderby_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_distinct_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_asc_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_desc_term(env_t *env, const protob_t<const Term> &term);

// time.cc
counted_t<term_t> make_iso8601_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_to_iso8601_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_epoch_time_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_to_epoch_time_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_now_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_in_timezone_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_during_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_date_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_time_of_day_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_timezone_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_time_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_portion_term(env_t *env, const protob_t<const Term> &term,
                                    pseudo::time_component_t component);

// type_manip.cc
counted_t<term_t> make_coerce_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_typeof_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_info_term(env_t *env, const protob_t<const Term> &term);

// var.cc
counted_t<term_t> make_var_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_implicit_var_term(env_t *env, const protob_t<const Term> &term);

// writes.cc
counted_t<term_t> make_insert_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_replace_term(env_t *env, const protob_t<const Term> &term);
counted_t<term_t> make_foreach_term(env_t *env, const protob_t<const Term> &term);

} // namespace ql

#endif  // RDB_PROTOCOL_TERMS_TERMS_HPP_
