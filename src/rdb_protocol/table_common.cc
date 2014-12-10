// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/table_common.hpp"

std::string error_message_index_not_found(
        const std::string &sindex, const std::string &table) {
    return strprintf("Index `%s` was not found on table `%s`",
        sindex.c_str(), table.c_str());
}

namespace {

MUST_USE ql::datum_t
make_replacement_pair(ql::datum_t old_val, ql::datum_t new_val) {
    // in this context, we know the array will have one element.
    // stats_merge later can impose user preferences.
    ql::datum_array_builder_t values(ql::configured_limits_t::unlimited);
    ql::datum_object_builder_t value_pair;
    bool conflict = value_pair.add("old_val", old_val)
        || value_pair.add("new_val", new_val);
    guarantee(!conflict);
    values.add(std::move(value_pair).to_datum());
    return std::move(values).to_datum();
}

}   /* anonymous namespace */

/* TODO: This looks an awful lot like `rcheck_valid_replace()`. Perhaps they should be
combined. */
void rcheck_row_replacement(
        const datum_string_t &primary_key_name,
        const store_key_t &primary_key_value,
        ql::datum_t old_row,
        ql::datum_t new_row) {
    if (new_row.get_type() == ql::datum_t::R_OBJECT) {
        new_row.rcheck_valid_replace(
            old_row, ql::datum_t(), primary_key_name);
        ql::datum_t new_primary_key_value =
            new_row.get_field(primary_key_name, ql::NOTHROW);
        rcheck_target(&new_row,
            primary_key_value.compare(
                store_key_t(new_primary_key_value.print_primary())) == 0,
            ql::base_exc_t::GENERIC,
            (old_row.get_type() == ql::datum_t::R_NULL
             ? strprintf("Primary key `%s` cannot be changed (null -> %s)",
                         primary_key_name.to_std().c_str(), new_row.print().c_str())
             : strprintf("Primary key `%s` cannot be changed (%s -> %s)",
                         primary_key_name.to_std().c_str(),
                         old_row.print().c_str(), new_row.print().c_str())));
    } else {
        rcheck_typed_target(&new_row,
            new_row.get_type() == ql::datum_t::R_NULL,
            strprintf("Inserted value must be an OBJECT (got %s):\n%s",
                new_row.get_type_name().c_str(), new_row.print().c_str()));
    }
}

ql::datum_t make_row_replacement_stats(
        const datum_string_t &primary_key_name,
        DEBUG_VAR const store_key_t &primary_key_value,
        ql::datum_t old_row,
        ql::datum_t new_row,
        return_changes_t return_changes,
        bool *was_changed_out) {

#ifndef NDEBUG
    try {
        rcheck_row_replacement(primary_key_name, primary_key_value, old_row, new_row);
    } catch (const ql::base_exc_t &) {
        crash("Called make_row_replacement_stats() with invalid parameters. Use "
            "rcheck_row_replacement() to validate parameters before calling "
            "make_row_replacement_stats().");
    }
#endif

    guarantee(old_row.has());
    bool started_empty;
    if (old_row.get_type() == ql::datum_t::R_NULL) {
        started_empty = true;
    } else if (old_row.get_type() == ql::datum_t::R_OBJECT) {
        started_empty = false;
#ifndef NDEBUG
        ql::datum_t old_row_pval = old_row.get_field(primary_key_name);
        rassert(old_row_pval.has());
        rassert(store_key_t(old_row_pval.print_primary()) == primary_key_value);
#endif
    } else {
        crash("old_row is invalid");
    }
    
    guarantee(new_row.has());
    bool ended_empty = (new_row.get_type() == ql::datum_t::R_NULL);

    *was_changed_out = (old_row != new_row);

    ql::datum_object_builder_t resp;
    if (return_changes == return_changes_t::YES) {
        bool conflict = resp.add("changes", make_replacement_pair(old_row, new_row));
        guarantee(!conflict);
    }

    // We use `conflict` below to store whether or not there was a key
    // conflict when constructing the stats object.  It defaults to `true`
    // so that we fail an assertion if we never update the stats object.
    bool conflict = true;

    // Figure out what operation we're doing (based on started_empty,
    // ended_empty, and the result of the function call) and then do it.
    if (started_empty) {
        if (ended_empty) {
            conflict = resp.add("skipped", ql::datum_t(1.0));
        } else {
            conflict = resp.add("inserted", ql::datum_t(1.0));
        }
    } else {
        if (ended_empty) {
            conflict = resp.add("deleted", ql::datum_t(1.0));
        } else {
            r_sanity_check(old_row.get_field(primary_key_name) ==
                           new_row.get_field(primary_key_name));
            if (!*was_changed_out) {
                conflict = resp.add("unchanged", ql::datum_t(1.0));
            } else {
                conflict = resp.add("replaced", ql::datum_t(1.0));
            }
        }
    }
    guarantee(!conflict); // message never added twice

    return std::move(resp).to_datum();
}

ql::datum_t make_row_replacement_error_stats(
        ql::datum_t old_row,
        return_changes_t return_changes,
        const char *error_message) {
    ql::datum_object_builder_t resp;
    if (return_changes == return_changes_t::YES) {
        bool conflict = resp.add("changes", make_replacement_pair(old_row, old_row));
        guarantee(!conflict);
    }
    resp.add_error(error_message);
    return std::move(resp).to_datum();
}

ql::datum_t resolve_insert_conflict(
        const std::string &primary_key,
        ql::datum_t old_row,
        ql::datum_t insert_row,
        conflict_behavior_t conflict_behavior) {
    if (old_row.get_type() == ql::datum_t::R_NULL) {
        return insert_row;
    } else if (conflict_behavior == conflict_behavior_t::REPLACE) {
        return insert_row;
    } else if (conflict_behavior == conflict_behavior_t::UPDATE) {
        return old_row.merge(insert_row);
    } else {
        rfail_target(&old_row, ql::base_exc_t::GENERIC,
                     "Duplicate primary key `%s`:\n%s\n%s",
                     primary_key.c_str(), old_row.print().c_str(),
                     insert_row.print().c_str());
    }
}

