// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/val.hpp"

#include "containers/name_string.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term.hpp"
#include "stl_utils.hpp"

namespace ql {

table_t::table_t(scoped_ptr_t<base_table_t> &&_table,
                 counted_t<const db_t> _db, const std::string &_name,
                 bool _use_outdated, const protob_t<const Backtrace> &backtrace)
    : pb_rcheckable_t(backtrace),
      db(_db),
      name(_name),
      table(std::move(_table)),
      use_outdated(_use_outdated),
      bounds(datum_range_t::universe()),
      sorting(sorting_t::UNORDERED)
{ }

counted_t<const datum_t> table_t::make_error_datum(const base_exc_t &exception) {
    datum_object_builder_t d;
    d.add_error(exception.what());
    return std::move(d).to_counted();
}

counted_t<const datum_t> table_t::batched_replace(
    env_t *env,
    const std::vector<counted_t<const datum_t> > &vals,
    const std::vector<counted_t<const datum_t> > &keys,
    counted_t<const func_t> replacement_generator,
    bool nondeterministic_replacements_ok,
    durability_requirement_t durability_requirement,
    return_changes_t return_changes) {
    r_sanity_check(vals.size() == keys.size());

    if (vals.empty()) {
        return ql::datum_t::empty_object();
    }

    if (!replacement_generator->is_deterministic()) {
        r_sanity_check(nondeterministic_replacements_ok);
        datum_object_builder_t stats;
        std::vector<counted_t<const datum_t> > replacement_values;
        replacement_values.reserve(vals.size());
        for (size_t i = 0; i < vals.size(); ++i) {
            counted_t<const datum_t> new_val;
            try {
                new_val = replacement_generator->call(env, vals[i])->as_datum();
                new_val->rcheck_valid_replace(vals[i], keys[i], get_pkey());
                r_sanity_check(new_val.has());
                replacement_values.push_back(new_val);
            } catch (const base_exc_t &e) {
                stats.add_error(e.what());
            }
        }
        counted_t<const datum_t> insert_stats = batched_insert(
            env, std::move(replacement_values), conflict_behavior_t::REPLACE,
            durability_requirement, return_changes);
        std::set<std::string> conditions;
        counted_t<const datum_t> merged
            = std::move(stats).to_counted()->merge(insert_stats, stats_merge,
                                                   env->limits, &conditions);
        datum_object_builder_t result(std::move(merged)->as_object());
        result.add_warnings(conditions, env->limits);
        return std::move(result).to_counted();
    } else {
        return table->write_batched_replace(
            env, keys, replacement_generator, return_changes,
            durability_requirement);
    }
}

counted_t<const datum_t> table_t::batched_insert(
    env_t *env,
    std::vector<counted_t<const datum_t> > &&insert_datums,
    conflict_behavior_t conflict_behavior,
    durability_requirement_t durability_requirement,
    return_changes_t return_changes) {

    datum_object_builder_t stats;
    std::vector<counted_t<const datum_t> > valid_inserts;
    valid_inserts.reserve(insert_datums.size());
    for (auto it = insert_datums.begin(); it != insert_datums.end(); ++it) {
        try {
            (*it)->rcheck_valid_replace(counted_t<const datum_t>(),
                                        counted_t<const datum_t>(),
                                        get_pkey());
            counted_t<const ql::datum_t> keyval = (*it)->get(get_pkey(), ql::NOTHROW);
            (*it)->get(get_pkey())->print_primary(); // does error checking
            valid_inserts.push_back(std::move(*it));
        } catch (const base_exc_t &e) {
            stats.add_error(e.what());
        }
    }

    if (valid_inserts.empty()) {
        return std::move(stats).to_counted();
    }

    counted_t<const datum_t> insert_stats =
        table->write_batched_insert(
            env, std::move(valid_inserts), conflict_behavior, return_changes,
            durability_requirement);
    std::set<std::string> conditions;
    counted_t<const datum_t> merged
        = std::move(stats).to_counted()->merge(insert_stats, stats_merge,
                                               env->limits, &conditions);
    datum_object_builder_t result(std::move(merged)->as_object());
    result.add_warnings(conditions, env->limits);
    return std::move(result).to_counted();
}

MUST_USE bool table_t::sindex_create(env_t *env,
                                     const std::string &id,
                                     counted_t<const func_t> index_func,
                                     sindex_multi_bool_t multi,
                                     sindex_geo_bool_t geo) {
    index_func->assert_deterministic("Index functions must be deterministic.");
    return table->sindex_create(env, id, index_func, multi, geo);
}

MUST_USE bool table_t::sindex_drop(env_t *env, const std::string &id) {
    return table->sindex_drop(env, id);
}

MUST_USE sindex_rename_result_t table_t::sindex_rename(env_t *env,
                                                       const std::string &old_name,
                                                       const std::string &new_name,
                                                       bool overwrite) {
    return table->sindex_rename(env, old_name, new_name, overwrite);
}

counted_t<const datum_t> table_t::sindex_list(env_t *env) {
    std::vector<std::string> sindexes = table->sindex_list(env);
    std::vector<counted_t<const datum_t> > array;
    array.reserve(sindexes.size());
    for (std::vector<std::string>::const_iterator it = sindexes.begin();
         it != sindexes.end(); ++it) {
        array.push_back(make_counted<datum_t>(std::string(*it)));
    }
    return make_counted<datum_t>(std::move(array), env->limits);
}

counted_t<const datum_t> table_t::sindex_status(env_t *env,
        std::set<std::string> sindexes) {
    std::map<std::string, counted_t<const datum_t> > statuses =
        table->sindex_status(env, sindexes);
    std::vector<counted_t<const datum_t> > array;
    for (auto it = statuses.begin(); it != statuses.end(); ++it) {
        r_sanity_check(std_contains(sindexes, it->first) || sindexes.empty());
        sindexes.erase(it->first);
        std::map<std::string, counted_t<const datum_t> > status =
            it->second->as_object();
        std::string index_name = it->first;
        status["index"] = make_counted<const datum_t>(std::move(index_name));
        array.push_back(make_counted<const datum_t>(std::move(status)));
    }
    rcheck(sindexes.empty(), base_exc_t::GENERIC,
           strprintf("Index `%s` was not found on table `%s`.",
                     sindexes.begin()->c_str(),
                     display_name().c_str()));
    return make_counted<const datum_t>(std::move(array), env->limits);
}

MUST_USE bool table_t::sync(env_t *env, const rcheckable_t *parent) {
    rcheck_target(parent, base_exc_t::GENERIC,
                  bounds.is_universe() && sorting == sorting_t::UNORDERED,
                  "sync can only be applied directly to a table.");
    // In order to get the guarantees that we expect from a user-facing command,
    // we always have to use hard durability in combination with sync.
    return sync_depending_on_durability(env, DURABILITY_REQUIREMENT_HARD);
}

MUST_USE bool table_t::sync_depending_on_durability(env_t *env,
                durability_requirement_t durability_requirement) {
    return table->write_sync_depending_on_durability(
        env, durability_requirement);
}

const std::string &table_t::get_pkey() {
    return table->get_pkey();
}

counted_t<const datum_t> table_t::get_row(env_t *env, counted_t<const datum_t> pval) {
    return table->read_row(env, pval, use_outdated);
}

counted_t<datum_stream_t> table_t::get_all(
        env_t *env,
        counted_t<const datum_t> value,
        const std::string &get_all_sindex_id,
        const protob_t<const Backtrace> &bt) {
    rcheck_src(bt.get(), base_exc_t::GENERIC, !sindex_id,
               "Cannot chain get_all and other indexed operations.");
    r_sanity_check(sorting == sorting_t::UNORDERED);
    r_sanity_check(bounds.is_universe());
    return table->read_all(
        env,
        get_all_sindex_id,
        bt,
        display_name(),
        datum_range_t(value),
        sorting_t::UNORDERED,
        use_outdated);
}

void table_t::add_sorting(const std::string &new_sindex_id, sorting_t _sorting,
                          const rcheckable_t *parent) {
    r_sanity_check(_sorting != sorting_t::UNORDERED);

    rcheck_target(parent, base_exc_t::GENERIC, sorting == sorting_t::UNORDERED,
            "Cannot apply 2 indexed orderings to the same TABLE.");
    rcheck_target(parent, base_exc_t::GENERIC, !sindex_id || *sindex_id == new_sindex_id,
            strprintf(
                "Cannot use 2 indexes in the same operation. Trying to use %s and %s",
                sindex_id->c_str(), new_sindex_id.c_str()));

    sindex_id = new_sindex_id;
    sorting = _sorting;
}

void table_t::add_bounds(datum_range_t &&new_bounds, const std::string &new_sindex_id,
                         const rcheckable_t *parent) {
    if (sindex_id) {
        rcheck_target(
            parent, base_exc_t::GENERIC, *sindex_id == new_sindex_id,
            strprintf(
                "Cannot use 2 indexes in the same operation.  Trying to use %s and %s.",
                sindex_id->c_str(), new_sindex_id.c_str()));
    } else {
        sindex_id = new_sindex_id;
    }

    rcheck_target(parent, base_exc_t::GENERIC, bounds.is_universe(),
                  "Cannot chain multiple betweens to the same table.");
    bounds = std::move(new_bounds);
}

counted_t<datum_stream_t> table_t::as_datum_stream(env_t *env,
        const protob_t<const Backtrace> &bt) {
    return table->read_all(
        env,
        (sindex_id ? *sindex_id : get_pkey()),
        bt,
        display_name(),
        bounds,
        sorting,
        use_outdated);
}

counted_t<datum_stream_t> table_t::get_intersecting(
        env_t *env,
        const counted_t<const datum_t> &query_geometry,
        const std::string &new_sindex_id,
        const pb_rcheckable_t *parent) {
    rcheck_target(parent, base_exc_t::GENERIC, !sindex_id,
                  "Cannot chain get_intersecting with other indexed operations.");
    rcheck_target(parent, base_exc_t::GENERIC, new_sindex_id != get_pkey(),
                  "get_intersecting cannot use the primary index.");
    sindex_id = new_sindex_id;
    r_sanity_check(sorting == sorting_t::UNORDERED);
    r_sanity_check(bounds.is_universe());

    return table->read_intersecting(
        env,
        *sindex_id,
        parent->backtrace(),
        display_name(),
        use_outdated,
        query_geometry);
}

counted_t<datum_stream_t> table_t::get_nearest(
        env_t *env,
        lat_lon_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const std::string &new_sindex_id,
        const pb_rcheckable_t *parent,
        const configured_limits_t &limits) {
    rcheck_target(parent, base_exc_t::GENERIC, !sindex_id,
                  "Cannot chain get_nearest with other indexed operations.");
    rcheck_target(parent, base_exc_t::GENERIC, new_sindex_id != get_pkey(),
                  "get_nearest cannot use the primary index.");
    sindex_id = new_sindex_id;
    r_sanity_check(sorting == sorting_t::UNORDERED);
    r_sanity_check(bounds.is_universe());

    return table->read_nearest(
        env,
        *sindex_id,
        parent->backtrace(),
        display_name(),
        use_outdated,
        center,
        max_dist,
        max_results,
        geo_system,
        dist_unit,
        limits);
}

val_t::type_t::type_t(val_t::type_t::raw_type_t _raw_type) : raw_type(_raw_type) { }

// NOTE: This *MUST* be kept in sync with the surrounding code (not that it
// should have to change very often).
bool raw_type_is_convertible(val_t::type_t::raw_type_t _t1,
                             val_t::type_t::raw_type_t _t2) {
    const int t1 = _t1, t2 = _t2,
        DB               = val_t::type_t::DB,
        TABLE            = val_t::type_t::TABLE,
        SELECTION        = val_t::type_t::SELECTION,
        SEQUENCE         = val_t::type_t::SEQUENCE,
        SINGLE_SELECTION = val_t::type_t::SINGLE_SELECTION,
        DATUM            = val_t::type_t::DATUM,
        FUNC             = val_t::type_t::FUNC,
        GROUPED_DATA     = val_t::type_t::GROUPED_DATA;
    switch (t1) {
    case DB:               return t2 == DB;
    case TABLE:            return t2 == TABLE || t2 == SELECTION || t2 == SEQUENCE;
    case SELECTION:        return t2 == SELECTION || t2 == SEQUENCE;
    case SEQUENCE:         return t2 == SEQUENCE;
    case SINGLE_SELECTION: return t2 == SINGLE_SELECTION || t2 == DATUM;
    case DATUM:            return t2 == DATUM || t2 == SEQUENCE;
    case FUNC:             return t2 == FUNC;
    case GROUPED_DATA:     return t2 == GROUPED_DATA;
    default: unreachable();
    }
}
bool val_t::type_t::is_convertible(type_t rhs) const {
    return raw_type_is_convertible(raw_type, rhs.raw_type);
}

const char *val_t::type_t::name() const {
    switch (raw_type) {
    case DB: return "DATABASE";
    case TABLE: return "TABLE";
    case SELECTION: return "SELECTION";
    case SEQUENCE: return "SEQUENCE";
    case SINGLE_SELECTION: return "SINGLE_SELECTION";
    case DATUM: return "DATUM";
    case FUNC: return "FUNCTION";
    case GROUPED_DATA: return "GROUPED_DATA";
    default: unreachable();
    }
}

val_t::val_t(counted_t<const datum_t> _datum, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::DATUM),
      u(_datum) {
    guarantee(datum().has());
}

val_t::val_t(const counted_t<grouped_data_t> &groups,
             protob_t<const Backtrace> bt)
    : pb_rcheckable_t(bt),
      type(type_t::GROUPED_DATA),
      u(groups) {
    guarantee(groups.has());
}

val_t::val_t(counted_t<const datum_t> _datum, counted_t<table_t> _table,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SINGLE_SELECTION),
      table(_table),
      u(_datum) {
    guarantee(table.has());
    guarantee(datum().has());
}

val_t::val_t(counted_t<const datum_t> _datum,
             counted_t<const datum_t> _orig_key,
             counted_t<table_t> _table,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SINGLE_SELECTION),
      table(_table),
      orig_key(_orig_key),
      u(_datum) {
    guarantee(table.has());
    guarantee(datum().has());
}

val_t::val_t(env_t *env, counted_t<datum_stream_t> _sequence,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SEQUENCE),
      u(_sequence) {
    guarantee(sequence().has());
    // Some streams are really arrays in disguise.
    counted_t<const datum_t> arr = sequence()->as_array(env);
    if (arr.has()) {
        type = type_t::DATUM;
        u = arr;
    }
}

val_t::val_t(counted_t<table_t> _table,
             counted_t<datum_stream_t> _sequence,
             protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::SELECTION),
      table(_table),
      u(_sequence) {
    guarantee(table.has());
    guarantee(sequence().has());
}

val_t::val_t(counted_t<table_t> _table, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::TABLE),
      table(_table) {
    guarantee(table.has());
}
val_t::val_t(counted_t<const db_t> _db, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::DB),
      u(_db) {
    guarantee(db().has());
}
val_t::val_t(counted_t<const func_t> _func, protob_t<const Backtrace> backtrace)
    : pb_rcheckable_t(backtrace),
      type(type_t::FUNC),
      u(_func) {
    guarantee(func().has());
}

val_t::~val_t() { }

val_t::type_t val_t::get_type() const { return type; }
const char * val_t::get_type_name() const { return get_type().name(); }

counted_t<const datum_t> val_t::as_datum() const {
    if (type.raw_type != type_t::DATUM && type.raw_type != type_t::SINGLE_SELECTION) {
        rcheck_literal_type(type_t::DATUM);
    }
    return datum();
}

counted_t<table_t> val_t::as_table() {
    rcheck_literal_type(type_t::TABLE);
    return table;
}

counted_t<datum_stream_t> val_t::as_seq(env_t *env) {
    if (type.raw_type == type_t::SEQUENCE || type.raw_type == type_t::SELECTION) {
        return sequence();
    } else if (type.raw_type == type_t::TABLE) {
        return table->as_datum_stream(env, backtrace());
    } else if (type.raw_type == type_t::DATUM) {
        return datum()->as_datum_stream(backtrace());
    }
    rcheck_literal_type(type_t::SEQUENCE);
    unreachable();
}

counted_t<grouped_data_t> val_t::as_grouped_data() {
    rcheck_literal_type(type_t::GROUPED_DATA);
    return boost::get<counted_t<grouped_data_t> >(u);
}

counted_t<grouped_data_t> val_t::as_promiscuous_grouped_data(env_t *env) {
    return ((type.raw_type == type_t::SEQUENCE) && sequence()->is_grouped())
        ? sequence()->to_array(env)->as_grouped_data()
        : as_grouped_data();
}

counted_t<grouped_data_t> val_t::maybe_as_grouped_data() {
    return (type.raw_type == type_t::GROUPED_DATA)
        ? as_grouped_data()
        : counted_t<grouped_data_t>();
}

counted_t<grouped_data_t> val_t::maybe_as_promiscuous_grouped_data(env_t *env) {
    return ((type.raw_type == type_t::SEQUENCE) && sequence()->is_grouped())
        ? sequence()->to_array(env)->as_grouped_data()
        : maybe_as_grouped_data();
}

std::pair<counted_t<table_t>, counted_t<datum_stream_t> >
val_t::as_selection(env_t *env) {
    if (type.raw_type != type_t::TABLE && type.raw_type != type_t::SELECTION) {
        rcheck_literal_type(type_t::SELECTION);
    }
    return std::make_pair(table, as_seq(env));
}

std::pair<counted_t<table_t>, counted_t<const datum_t> > val_t::as_single_selection() {
    rcheck_literal_type(type_t::SINGLE_SELECTION);
    return std::make_pair(table, datum());
}

counted_t<const func_t> val_t::as_func(function_shortcut_t shortcut) {
    if (get_type().is_convertible(type_t::FUNC)) {
        r_sanity_check(func().has());
        return func();
    }

    if (shortcut == NO_SHORTCUT) {
        rcheck_literal_type(type_t::FUNC);
        unreachable();
    }

    if (!type.is_convertible(type_t::DATUM)) {
        rcheck_literal_type(type_t::FUNC);
        unreachable();
    }

    try {
        switch (shortcut) {
        case CONSTANT_SHORTCUT:
            return new_constant_func(as_datum(), backtrace());
        case GET_FIELD_SHORTCUT:
            return new_get_field_func(as_datum(), backtrace());
        case PLUCK_SHORTCUT:
            return new_pluck_func(as_datum(), backtrace());
        case PAGE_SHORTCUT:
            return new_page_func(as_datum(), backtrace());
        case NO_SHORTCUT:
            // fallthru
        default: unreachable();
        }
    } catch (const datum_exc_t &ex) {
        throw exc_t(ex, backtrace().get());
    }
}

counted_t<const db_t> val_t::as_db() const {
    rcheck_literal_type(type_t::DB);
    return db();
}

counted_t<const datum_t> val_t::as_ptype(const std::string s) {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        d->rcheck_is_ptype(s);
        return d;
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}

bool val_t::as_bool() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_bool();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
double val_t::as_num() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_num();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
int64_t val_t::as_int() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_int();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
const wire_string_t &val_t::as_str() {
    try {
        counted_t<const datum_t> d = as_datum();
        r_sanity_check(d.has());
        return d->as_str();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}

void val_t::rcheck_literal_type(type_t::raw_type_t expected_raw_type) const {
    rcheck_typed_target(
        this, type.raw_type == expected_raw_type,
        strprintf("Expected type %s but found %s:\n%s",
                  type_t(expected_raw_type).name(), type.name(), print().c_str()));
}

std::string val_t::print() const {
    if (get_type().is_convertible(type_t::DATUM)) {
        return as_datum()->print();
    } else if (get_type().is_convertible(type_t::DB)) {
        return strprintf("db(\"%s\")", as_db()->name.c_str());
    } else if (get_type().is_convertible(type_t::TABLE)) {
        return strprintf("table(\"%s\")", table->name.c_str());
    } else if (get_type().is_convertible(type_t::SELECTION)) {
        return strprintf("SELECTION ON table(%s)",
                         table->name.c_str());
    } else {
        // TODO: Do something smarter here?
        return strprintf("VALUE %s", get_type().name());
    }
}

std::string val_t::trunc_print() const {
    if (get_type().is_convertible(type_t::DATUM)) {
        return as_datum()->trunc_print();
    } else {
        std::string s = print();
        if (s.size() > datum_t::trunc_len) {
            s.erase(s.begin() + (datum_t::trunc_len - 3), s.end());
            s += "...";
        }
        return s;
    }
}

counted_t<const datum_t> val_t::get_orig_key() const {
    return orig_key;
}

} // namespace ql
