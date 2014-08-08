// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "containers/counted.hpp"
#include "containers/wire_string.hpp"
#include "rdb_protocol/geo/distances.hpp"
#include "rdb_protocol/geo/lat_lon_types.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"

class ellipsoid_spec_t;
namespace ql {
class datum_t;
class env_t;
template <class> class protob_t;
class scope_env_t;
class stream_cache_t;
class term_t;
class val_t;

/* A `table_t` is an `r.table` term, possibly with some other things chained onto
onto it. */
class table_t : public single_threaded_countable_t<table_t>, public pb_rcheckable_t {
public:
    table_t(scoped_ptr_t<base_table_t> &&,
            counted_t<const db_t> db, const std::string &name,
            bool use_outdated, const protob_t<const Backtrace> &src);
    counted_t<datum_stream_t> as_datum_stream(env_t *env,
                                              const protob_t<const Backtrace> &bt);
    const std::string &get_pkey();
    counted_t<const datum_t> get_row(env_t *env, counted_t<const datum_t> pval);
    counted_t<datum_stream_t> get_all(
            env_t *env,
            counted_t<const datum_t> value,
            const std::string &sindex_id,
            const protob_t<const Backtrace> &bt);
    void add_sorting(
            const std::string &sindex_id, sorting_t sorting,
            const rcheckable_t *parent);
    void add_bounds(
            datum_range_t &&new_bounds,
            const std::string &new_sindex_id,
            const rcheckable_t *parent);
    counted_t<datum_stream_t> get_intersecting(
            env_t *env,
            const counted_t<const datum_t> &query_geometry,
            const std::string &new_sindex_id,
            const pb_rcheckable_t *parent);
    counted_t<datum_stream_t> get_nearest(
            env_t *env,
            lat_lon_point_t center,
            double max_dist,
            uint64_t max_results,
            const ellipsoid_spec_t &geo_system,
            dist_unit_t dist_unit,
            const std::string &new_sindex_id,
            const pb_rcheckable_t *parent,
            const configured_limits_t &limits);

    counted_t<const datum_t> make_error_datum(const base_exc_t &exception);

    counted_t<const datum_t> batched_replace(
        env_t *env,
        const std::vector<counted_t<const datum_t> > &vals,
        const std::vector<counted_t<const datum_t> > &keys,
        counted_t<func_t> replacement_generator,
        bool nondeterministic_replacements_ok,
        durability_requirement_t durability_requirement,
        return_changes_t return_changes);

    counted_t<const datum_t> batched_insert(
        env_t *env,
        std::vector<counted_t<const datum_t> > &&insert_datums,
        conflict_behavior_t conflict_behavior,
        durability_requirement_t durability_requirement,
        return_changes_t return_changes);

    MUST_USE bool sindex_create(
        env_t *env, const std::string &name,
        counted_t<func_t> index_func, sindex_multi_bool_t multi,
        sindex_geo_bool_t geo);
    MUST_USE bool sindex_drop(env_t *env, const std::string &name);
    MUST_USE sindex_rename_result_t sindex_rename(
        env_t *env, const std::string &old_name,
        const std::string &new_name, bool overwrite);
    counted_t<const datum_t> sindex_list(env_t *env);
    counted_t<const datum_t> sindex_status(env_t *env,
        std::set<std::string> sindex);
    MUST_USE bool sync(env_t *env, const rcheckable_t *parent);

    /* `db` and `name` are for display purposes only */
    counted_t<const db_t> db;
    const std::string name;
    std::string display_name() {
        return db->name + "." + name;
    }

    scoped_ptr_t<base_table_t> table;

private:
    friend class distinct_term_t;

    counted_t<const datum_t> batched_insert_with_keys(
        env_t *env,
        const std::vector<store_key_t> &keys,
        const std::vector<counted_t<const datum_t> > &insert_datums,
        conflict_behavior_t conflict_behavior,
        durability_requirement_t durability_requirement);

    MUST_USE bool sync_depending_on_durability(
        env_t *env, durability_requirement_t durability_requirement);

    bool use_outdated;

    boost::optional<std::string> sindex_id;

    datum_range_t bounds;
    sorting_t sorting;
};


enum function_shortcut_t {
    NO_SHORTCUT = 0,
    CONSTANT_SHORTCUT = 1,
    GET_FIELD_SHORTCUT = 2,
    PLUCK_SHORTCUT = 3,
    PAGE_SHORTCUT = 4
};

// A value is anything RQL can pass around -- a datum, a sequence, a function, a
// selection, whatever.
class val_t : public single_threaded_countable_t<val_t>, public pb_rcheckable_t {
public:
    // This type is intentionally opaque.  It is almost always an error to
    // compare two `val_t` types rather than testing whether one is convertible
    // to another.
    class type_t {
        friend class val_t;
        friend void run(Query *q, scoped_ptr_t<env_t> *env_ptr,
                        Response *res, stream_cache_t *stream_cache,
                        bool *response_needed_out);
    public:
        enum raw_type_t {
            DB               = 1, // db
            TABLE            = 2, // table
            SELECTION        = 3, // table, sequence
            SEQUENCE         = 4, // sequence
            SINGLE_SELECTION = 5, // table, datum (object)
            DATUM            = 6, // datum
            FUNC             = 7, // func
            GROUPED_DATA     = 8  // grouped_data
        };
        type_t(raw_type_t _raw_type); // NOLINT
        bool is_convertible(type_t rhs) const;

        raw_type_t get_raw_type() const { return raw_type; }
        const char *name() const;

    private:
        friend class coerce_term_t;
        friend class typeof_term_t;
        friend int val_type(counted_t<val_t> v);
        raw_type_t raw_type;
    };
    type_t get_type() const;
    const char *get_type_name() const;

    val_t(counted_t<const datum_t> _datum, protob_t<const Backtrace> backtrace);
    val_t(const counted_t<grouped_data_t> &groups,
          protob_t<const Backtrace> bt);
    val_t(counted_t<const datum_t> _datum, counted_t<table_t> _table,
          protob_t<const Backtrace> backtrace);
    val_t(counted_t<const datum_t> _datum,
          counted_t<const datum_t> _orig_key,
          counted_t<table_t> _table,
          protob_t<const Backtrace> backtrace);
    val_t(env_t *env, counted_t<datum_stream_t> _sequence,
          protob_t<const Backtrace> backtrace);
    val_t(counted_t<table_t> _table, protob_t<const Backtrace> backtrace);
    val_t(counted_t<table_t> _table, counted_t<datum_stream_t> _sequence,
          protob_t<const Backtrace> backtrace);
    val_t(counted_t<const db_t> _db, protob_t<const Backtrace> backtrace);
    val_t(counted_t<func_t> _func, protob_t<const Backtrace> backtrace);
    ~val_t();

    counted_t<const db_t> as_db() const;
    counted_t<table_t> as_table();
    std::pair<counted_t<table_t>, counted_t<datum_stream_t> > as_selection(env_t *env);
    counted_t<datum_stream_t> as_seq(env_t *env);
    std::pair<counted_t<table_t>, counted_t<const datum_t> > as_single_selection();
    // See func.hpp for an explanation of shortcut functions.
    counted_t<func_t> as_func(function_shortcut_t shortcut = NO_SHORTCUT);

    // This set of interfaces is atrocious.  Basically there are some places
    // where we want grouped_data, some places where we maybe want grouped_data,
    // and some places where we maybe want grouped data even if we have to
    // coerce to grouped data from a grouped stream.  (We can't use the usual
    // `is_convertible` interface because the type information is actually a
    // property of the stream, because I'm a terrible programmer.)
    counted_t<grouped_data_t> as_grouped_data();
    counted_t<grouped_data_t> as_promiscuous_grouped_data(env_t *env);
    counted_t<grouped_data_t> maybe_as_grouped_data();
    counted_t<grouped_data_t> maybe_as_promiscuous_grouped_data(env_t *env);

    counted_t<const datum_t> as_datum() const; // prefer the 4 below
    counted_t<const datum_t> as_ptype(const std::string s = "");
    bool as_bool();
    double as_num();
    template<class T>
    T as_int() {
        int64_t i = as_int();
        T t = static_cast<T>(i);
        rcheck(static_cast<int64_t>(t) == i,
               base_exc_t::GENERIC,
               strprintf("Integer too large: %" PRIi64, i));
        return t;
    }
    int64_t as_int();
    const wire_string_t &as_str();

    std::string print() const;
    std::string trunc_print() const;

    counted_t<const datum_t> get_orig_key() const;

private:
    friend int val_type(counted_t<val_t> v); // type_manip version
    void rcheck_literal_type(type_t::raw_type_t expected_raw_type) const;

    type_t type;
    counted_t<table_t> table;
    counted_t<const datum_t> orig_key;

    // We pretend that this variant is a union -- as if it doesn't have type
    // information.  The sequence, datum, func, and db_ptr functions get the
    // fields of the variant.
    boost::variant<counted_t<const db_t>,
                   counted_t<datum_stream_t>,
                   counted_t<const datum_t>,
                   counted_t<func_t>,
                   counted_t<grouped_data_t> > u;

    const counted_t<const db_t> &db() const {
        return boost::get<counted_t<const db_t> >(u);
    }
    counted_t<datum_stream_t> &sequence() {
        return boost::get<counted_t<datum_stream_t> >(u);
    }
    counted_t<const datum_t> &datum() {
        return boost::get<counted_t<const datum_t> >(u);
    }
    const counted_t<const datum_t> &datum() const {
        return boost::get<counted_t<const datum_t> >(u);
    }
    counted_t<func_t> &func() { return boost::get<counted_t<func_t> >(u); }

    DISABLE_COPYING(val_t);
};


}  // namespace ql

#endif // RDB_PROTOCOL_VAL_HPP_
