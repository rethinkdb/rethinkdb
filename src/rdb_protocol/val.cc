// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/val.hpp"

#include "containers/name_string.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/math_utils.hpp"
#include "rdb_protocol/minidriver.hpp"
#include "rdb_protocol/term.hpp"
#include "stl_utils.hpp"
#include "thread_local.hpp"

namespace ql {

class get_selection_t : public single_selection_t {
public:
    get_selection_t(env_t *_env,
                    backtrace_id_t _bt,
                    counted_t<table_t> _tbl,
                    datum_t _key,
                    datum_t _row = datum_t())
        : env(_env),
          bt(std::move(_bt)),
          tbl(std::move(_tbl)),
          key(std::move(_key)),
          row(std::move(_row)) { }
    virtual datum_t get() {
        if (!row.has()) {
            row = tbl->get_row(env, key);
        }
        return row;
    }
    virtual counted_t<datum_stream_t> read_changes(
        bool include_initial,
        configured_limits_t limits,
        const datum_t &squash,
        bool include_states) {
        counted_t<datum_stream_t> maybe_src;
        if (include_initial) {
            // We want to provide an empty stream in this case because we get
            // the initial values from the stamp read instead.
            maybe_src = make_counted<vector_datum_stream_t>(
                bt, std::vector<datum_t>(), boost::none);
        }
        return tbl->tbl->read_changes(
            env,
            maybe_src,
            std::move(limits),
            squash,
            include_states,
            changefeed::keyspec_t::point_t{key},
            bt,
            tbl->display_name());
    }
    virtual datum_t replace(
        counted_t<const func_t> f, bool nondet_ok,
        durability_requirement_t dur_req, return_changes_t return_changes) {
        std::vector<datum_t > keys{key};
        // We don't need to fetch the value for deterministic replacements.
        std::vector<datum_t > vals{
            f->is_deterministic() == deterministic_t::always ? datum_t() : get()};
        return tbl->batched_replace(
            env, vals, keys, f, nondet_ok, dur_req, return_changes);
    }
    virtual const counted_t<table_t> &get_tbl() { return tbl; }
private:
    env_t *env;
    backtrace_id_t bt;
    counted_t<table_t> tbl;
    datum_t key, row;
};

class extreme_selection_t : public single_selection_t {
public:
    extreme_selection_t(env_t *_env,
                        backtrace_id_t _bt,
                        counted_t<table_slice_t> _slice,
                        std::string _err)
        : env(_env),
          bt(std::move(_bt)),
          slice(std::move(_slice)),
          err(std::move(_err)) { }
    virtual datum_t get() {
        if (!row.has()) {
            batchspec_t batchspec = batchspec_t::all().with_at_most(1);
            row = slice->as_seq(env, bt)->next(env, batchspec);
            if (!row.has()) {
                rfail_src(bt, base_exc_t::LOGIC, "%s", err.c_str());
            }
        }
        return row;
    }
    virtual counted_t<datum_stream_t> read_changes(
        bool include_initial,
        configured_limits_t limits,
        const datum_t &squash,
        bool include_states) {
        changefeed::keyspec_t::spec_t spec =
            ql::changefeed::keyspec_t::limit_t{slice->get_range_spec(), 1};
        counted_t<datum_stream_t> maybe_src;
        if (include_initial) {
            // We want to provide an empty stream in this case because we get
            // the initial values from the stamp read instead.
            maybe_src = make_counted<vector_datum_stream_t>(
                bt, std::vector<datum_t>(), boost::none);
        }
        auto s = slice->get_tbl()->tbl->read_changes(
            env,
            maybe_src,
            std::move(limits),
            squash,
            include_states,
            std::move(spec),
            bt,
            slice->get_tbl()->display_name());
        return s;
    }
    virtual datum_t replace(
        counted_t<const func_t> f, bool nondet_ok,
        durability_requirement_t dur_req, return_changes_t return_changes) {
        std::vector<datum_t > vals{get()};
        std::vector<datum_t > keys{
            vals[0].get_field(
                datum_string_t(get_tbl()->get_pkey()),
                NOTHROW)};
        r_sanity_check(keys[0].has());
        return slice->get_tbl()->batched_replace(
            env, vals, keys, f, nondet_ok, dur_req, return_changes);
    }
    virtual const counted_t<table_t> &get_tbl() { return slice->get_tbl(); }
private:
    env_t *env;
    backtrace_id_t bt;
    counted_t<table_slice_t> slice;
    datum_t row;
    std::string err;
};

counted_t<single_selection_t> single_selection_t::from_key(
    env_t *env, backtrace_id_t bt,
    counted_t<table_t> table, datum_t key) {
    return make_counted<get_selection_t>(
        env, std::move(bt), std::move(table), std::move(key));
}
counted_t<single_selection_t> single_selection_t::from_row(
    env_t *env, backtrace_id_t bt,
    counted_t<table_t> table, datum_t row) {
    datum_t d = row.get_field(datum_string_t(table->get_pkey()), NOTHROW);
    r_sanity_check(d.has());
    return make_counted<get_selection_t>(
        env, std::move(bt), std::move(table), std::move(d), std::move(row));
}
counted_t<single_selection_t> single_selection_t::from_slice(
    env_t *env, backtrace_id_t bt,
    counted_t<table_slice_t> table, std::string err) {
    return make_counted<extreme_selection_t>(
        env, std::move(bt), std::move(table), std::move(err));
}

table_slice_t::table_slice_t(counted_t<table_t> _tbl,
                             boost::optional<std::string> _idx,
                             sorting_t _sorting,
                             datum_range_t _bounds)
    : bt_rcheckable_t(_tbl->backtrace()),
      tbl(std::move(_tbl)), idx(std::move(_idx)),
      sorting(_sorting), bounds(std::move(_bounds)) { }


counted_t<datum_stream_t> table_slice_t::as_seq(
    env_t *env, backtrace_id_t bt) {
    // Empty bounds will be handled by as_seq with empty_reader_t
    return tbl->as_seq(env, idx ? *idx : tbl->get_pkey(), bt, bounds, sorting);
}

counted_t<table_slice_t>
table_slice_t::with_sorting(std::string _idx, sorting_t _sorting) {
    rcheck(sorting == sorting_t::UNORDERED, base_exc_t::LOGIC,
           "Cannot perform multiple indexed ORDER_BYs on the same table.");
    bool idx_legal = idx ? (*idx == _idx) : true;
    r_sanity_check(idx_legal || !bounds.is_universe());
    rcheck(idx_legal, base_exc_t::LOGIC,
           strprintf("Cannot order by index `%s` after calling BETWEEN on index `%s`.",
                     _idx.c_str(), (*idx).c_str()));
    return make_counted<table_slice_t>(tbl, std::move(_idx), _sorting, bounds);
}

counted_t<table_slice_t>
table_slice_t::with_bounds(std::string _idx, datum_range_t _bounds) {
    rcheck(bounds.is_universe(), base_exc_t::LOGIC,
           "Cannot perform multiple BETWEENs on the same table.");
    bool idx_legal = idx ? (*idx == _idx) : true;
    r_sanity_check(idx_legal || sorting != sorting_t::UNORDERED);
    rcheck(idx_legal, base_exc_t::LOGIC,
           strprintf("Cannot call BETWEEN on index `%s` after ordering on index `%s`.",
                     _idx.c_str(), (*idx).c_str()));
    return make_counted<table_slice_t>(
        tbl, std::move(_idx), sorting, std::move(_bounds));
}

ql::changefeed::keyspec_t::range_t table_slice_t::get_range_spec() {
    return ql::changefeed::keyspec_t::range_t{
        std::vector<transform_variant_t>(),
        idx && *idx == tbl->get_pkey() ? boost::none : idx,
        sorting,
        datumspec_t(bounds)};
}

counted_t<datum_stream_t> table_t::as_seq(
    env_t *env,
    const std::string &idx,
    backtrace_id_t bt,
    const datum_range_t &bounds,
    sorting_t sorting) {
    return tbl->read_all(
        env,
        idx,
        bt,
        display_name(),
        datumspec_t(bounds),
        sorting,
        read_mode);
}

table_t::table_t(counted_t<base_table_t> &&_tbl,
                 counted_t<const db_t> _db, const std::string &_name,
                 read_mode_t _read_mode, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      db(_db),
      name(_name),
      tbl(std::move(_tbl)),
      read_mode(_read_mode)
{ }

datum_t table_t::make_error_datum(const base_exc_t &exception) {
    datum_object_builder_t d;
    d.add_error(exception.what());
    return std::move(d).to_datum();
}

datum_t table_t::batched_replace(
    env_t *env,
    const std::vector<datum_t> &vals,
    const std::vector<datum_t> &keys,
    counted_t<const func_t> replacement_generator,
    bool nondeterministic_replacements_ok,
    durability_requirement_t durability_requirement,
    return_changes_t return_changes) {
    r_sanity_check(vals.size() == keys.size());

    if (vals.empty()) {
        return ql::datum_t::empty_object();
    }

    if (replacement_generator->is_deterministic() != deterministic_t::always) {
        r_sanity_check(nondeterministic_replacements_ok);
        datum_object_builder_t stats;
        std::vector<datum_t> replacement_values;
        replacement_values.reserve(vals.size());
        for (size_t i = 0; i < vals.size(); ++i) {
            r_sanity_check(vals[i].has());
            datum_t new_val;
            try {
                new_val = replacement_generator->call(env, vals[i])->as_datum();
                new_val.rcheck_valid_replace(vals[i], keys[i],
                                             datum_string_t(get_pkey()));
                r_sanity_check(new_val.has());
                replacement_values.push_back(new_val);
            } catch (const base_exc_t &e) {
                stats.add_error(e.what());
            }
        }
        std::vector<bool> pkey_was_autogenerated(vals.size(), false);
        datum_t insert_stats = batched_insert(
            env, std::move(replacement_values), std::move(pkey_was_autogenerated),
            conflict_behavior_t::REPLACE, durability_requirement, return_changes);
        std::set<std::string> conditions;
        datum_t merged
            = std::move(stats).to_datum().merge(insert_stats, stats_merge,
                                                 env->limits(), &conditions);
        datum_object_builder_t result(merged);
        result.add_warnings(conditions, env->limits());
        return std::move(result).to_datum();
    } else {
        return tbl->write_batched_replace(
            env, keys, replacement_generator, return_changes,
            durability_requirement);
    }
}

datum_t table_t::batched_insert(
    env_t *env,
    std::vector<datum_t> &&insert_datums,
    std::vector<bool> &&pkey_was_autogenerated,
    conflict_behavior_t conflict_behavior,
    durability_requirement_t durability_requirement,
    return_changes_t return_changes) {

    datum_object_builder_t stats;
    std::vector<datum_t> valid_inserts;
    valid_inserts.reserve(insert_datums.size());
    for (auto it = insert_datums.begin(); it != insert_datums.end(); ++it) {
        try {
            datum_string_t pkey_w(get_pkey());
            it->rcheck_valid_replace(datum_t(),
                                     datum_t(),
                                     pkey_w);
            const ql::datum_t &keyval = (*it).get_field(pkey_w);
            keyval.print_primary(); // does error checking
            valid_inserts.push_back(std::move(*it));
        } catch (const base_exc_t &e) {
            stats.add_error(e.what());
        }
    }

    if (valid_inserts.empty()) {
        return std::move(stats).to_datum();
    }

    datum_t insert_stats =
        tbl->write_batched_insert(
            env, std::move(valid_inserts), std::move(pkey_was_autogenerated),
            conflict_behavior, return_changes, durability_requirement);
    std::set<std::string> conditions;
    datum_t merged
        = std::move(stats).to_datum().merge(insert_stats, stats_merge,
                                             env->limits(), &conditions);
    datum_object_builder_t result(merged);
    result.add_warnings(conditions, env->limits());
    return std::move(result).to_datum();
}

MUST_USE bool table_t::sync(env_t *env) {
    // In order to get the guarantees that we expect from a user-facing command,
    // we always have to use hard durability in combination with sync.
    return sync_depending_on_durability(env, DURABILITY_REQUIREMENT_HARD);
}

MUST_USE bool table_t::sync_depending_on_durability(env_t *env,
                durability_requirement_t durability_requirement) {
    return tbl->write_sync_depending_on_durability(
        env, durability_requirement);
}

ql::datum_t table_t::get_id() const {
    return tbl->get_id();
}

const std::string &table_t::get_pkey() const {
    return tbl->get_pkey();
}

datum_t table_t::get_row(env_t *env, datum_t pval) {
    return tbl->read_row(env, pval, read_mode);
}

counted_t<datum_stream_t> table_t::get_all(
        env_t *env,
        const datumspec_t &datumspec,
        const std::string &get_all_sindex_id,
        backtrace_id_t bt) {
    return tbl->read_all(
        env,
        get_all_sindex_id,
        bt,
        display_name(),
        datumspec,
        sorting_t::UNORDERED,
        read_mode);
}

counted_t<datum_stream_t> table_t::get_intersecting(
        env_t *env,
        const datum_t &query_geometry,
        const std::string &new_sindex_id,
        const bt_rcheckable_t *parent) {
    return tbl->read_intersecting(
        env,
        new_sindex_id,
        parent->backtrace(),
        display_name(),
        read_mode,
        query_geometry);
}

datum_t table_t::get_nearest(
        env_t *env,
        lon_lat_point_t center,
        double max_dist,
        uint64_t max_results,
        const ellipsoid_spec_t &geo_system,
        dist_unit_t dist_unit,
        const std::string &new_sindex_id,
        const configured_limits_t &limits) {
    return tbl->read_nearest(
        env,
        new_sindex_id,
        display_name(),
        read_mode,
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
        TABLE_SLICE      = val_t::type_t::TABLE_SLICE,
        SELECTION        = val_t::type_t::SELECTION,
        SEQUENCE         = val_t::type_t::SEQUENCE,
        SINGLE_SELECTION = val_t::type_t::SINGLE_SELECTION,
        DATUM            = val_t::type_t::DATUM,
        FUNC             = val_t::type_t::FUNC,
        GROUPED_DATA     = val_t::type_t::GROUPED_DATA;
    switch (t1) {
    case DB:               return t2 == DB;
    case TABLE:            return t2 == TABLE || t2 == TABLE_SLICE
                                  || t2 == SELECTION || t2 == SEQUENCE;
    case TABLE_SLICE:      return t2 == TABLE_SLICE || t2 == SELECTION || t2 == SEQUENCE;
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
    case TABLE_SLICE: return "TABLE_SLICE";
    case SELECTION: return "SELECTION";
    case SEQUENCE: return "SEQUENCE";
    case SINGLE_SELECTION: return "SINGLE_SELECTION";
    case DATUM: return "DATUM";
    case FUNC: return "FUNCTION";
    case GROUPED_DATA: return "GROUPED_DATA";
    default: unreachable();
    }
}

val_t::val_t(datum_t _datum, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::DATUM),
      u(_datum) {
    guarantee(datum().has());
}

val_t::val_t(const counted_t<grouped_data_t> &groups,
             backtrace_id_t bt)
    : bt_rcheckable_t(bt),
      type(type_t::GROUPED_DATA),
      u(groups) {
    guarantee(groups.has());
}

val_t::val_t(counted_t<single_selection_t> _selection, backtrace_id_t bt)
    : bt_rcheckable_t(bt),
      type(type_t::SINGLE_SELECTION),
      u(_selection) {
    guarantee(single_selection().has());
}

val_t::val_t(env_t *env, counted_t<datum_stream_t> _sequence,
             backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::SEQUENCE),
      u(_sequence) {
    guarantee(sequence().has());
    // Some streams are really arrays in disguise.
    datum_t arr = sequence()->as_array(env);
    if (arr.has()) {
        type = type_t::DATUM;
        u = arr;
    }
}

val_t::val_t(counted_t<selection_t> _selection, backtrace_id_t bt)
    : bt_rcheckable_t(bt),
      type(type_t::SELECTION),
      u(_selection) {
    guarantee(selection().has());
}

val_t::val_t(counted_t<table_t> _table, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::TABLE),
      u(_table) {
    guarantee(table().has());
}
val_t::val_t(counted_t<table_slice_t> _slice, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::TABLE_SLICE),
      u(_slice) {
    guarantee(table_slice().has());
}
val_t::val_t(counted_t<const db_t> _db, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::DB),
      u(_db) {
    guarantee(db().has());
}
val_t::val_t(counted_t<const func_t> _func, backtrace_id_t backtrace)
    : bt_rcheckable_t(backtrace),
      type(type_t::FUNC),
      u(_func) {
    guarantee(func().has());
}

val_t::~val_t() { }

val_t::type_t val_t::get_type() const { return type; }
const char * val_t::get_type_name() const { return get_type().name(); }

datum_t val_t::as_datum() const {
    if (type.raw_type == type_t::DATUM) {
        return datum();
    } else if (type.raw_type == type_t::SINGLE_SELECTION) {
        return single_selection()->get();
    }
    rcheck_literal_type(type_t::DATUM);
    unreachable();
}

counted_t<table_t> val_t::as_table() {
    rcheck_literal_type(type_t::TABLE);
    return table();
}
counted_t<table_slice_t> val_t::as_table_slice() {
    if (type.raw_type == type_t::TABLE) {
        return make_counted<table_slice_t>(table());
    } else {
        rcheck_literal_type(type_t::TABLE_SLICE);
        return table_slice();
    }
}

counted_t<datum_stream_t> val_t::as_seq(env_t *env) {
    if (type.raw_type == type_t::SEQUENCE) {
        return sequence();
    } else if (type.raw_type == type_t::SELECTION) {
        return selection()->seq;
    } else if (type.raw_type == type_t::TABLE_SLICE || type.raw_type == type_t::TABLE) {
        return as_table_slice()->as_seq(env, backtrace());
    } else if (type.raw_type == type_t::DATUM) {
        return datum().as_datum_stream(backtrace());
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

counted_t<table_t> val_t::get_underlying_table() const {
    if (type.raw_type == type_t::TABLE) {
        return table();
    } else if (type.raw_type == type_t::SELECTION) {
        return selection()->table;
    } else if(type.raw_type == type_t::SINGLE_SELECTION) {
        return single_selection()->get_tbl();
    } else if (type.raw_type == type_t::TABLE_SLICE) {
        return table_slice()->get_tbl();
    } else {
        r_sanity_check(false);
        unreachable();
    }
}

counted_t<selection_t> val_t::as_selection(env_t *env) {
    if (type.raw_type == type_t::SELECTION) {
        return selection();
    } else if (type.is_convertible(type_t::TABLE_SLICE)) {
        counted_t<table_slice_t> slice = as_table_slice();
        return make_counted<selection_t>(
            slice->get_tbl(),
            slice->as_seq(env, backtrace()));
    }
    rcheck_literal_type(type_t::SELECTION);
    unreachable();
}

counted_t<single_selection_t> val_t::as_single_selection() {
    rcheck_literal_type(type_t::SINGLE_SELECTION);
    return single_selection();
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
        throw exc_t(ex, backtrace());
    }
}

counted_t<const db_t> val_t::as_db() const {
    rcheck_literal_type(type_t::DB);
    return db();
}

datum_t val_t::as_ptype(const std::string s) const {
    try {
        datum_t d = as_datum();
        r_sanity_check(d.has());
        d.rcheck_is_ptype(s);
        return d;
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}

bool val_t::as_bool() const {
    try {
        datum_t d = as_datum();
        r_sanity_check(d.has());
        return d.as_bool();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
double val_t::as_num() const {
    try {
        datum_t d = as_datum();
        r_sanity_check(d.has());
        return d.as_num();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
int64_t val_t::as_int() const {
    try {
        datum_t d = as_datum();
        r_sanity_check(d.has());
        return d.as_int();
    } catch (const datum_exc_t &e) {
        rfail(e.get_type(), "%s", e.what());
    }
}
datum_string_t val_t::as_str() const {
    try {
        datum_t d = as_datum();
        r_sanity_check(d.has());
        return d.as_str();
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
        return as_datum().print();
    } else if (get_type().is_convertible(type_t::DB)) {
        return strprintf("db(\"%s\")", as_db()->name.c_str());
    } else if (get_type().is_convertible(type_t::TABLE)) {
        return strprintf("table(\"%s\")", get_underlying_table()->name.c_str());
    } else if (get_type().is_convertible(type_t::SELECTION)) {
        return strprintf("SELECTION ON table(%s)",
                         get_underlying_table()->name.c_str());
    } else {
        // TODO: Do something smarter here?
        return strprintf("VALUE %s", get_type().name());
    }
}

std::string val_t::trunc_print() const {
    if (get_type().is_convertible(type_t::DATUM)) {
        return as_datum().trunc_print();
    } else {
        std::string s = print();
        if (s.size() > datum_t::trunc_len) {
            s.erase(s.begin() + (datum_t::trunc_len - 3), s.end());
            s += "...";
        }
        return s;
    }
}

} // namespace ql
