// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include <string>
#include <utility>
#include <vector>

#include "containers/counted.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream.hpp"

namespace ql {
class datum_t;
class env_t;
template <class> class protob_t;
class scope_env_t;
class stream_cache2_t;
class term_t;
class val_t;

class db_t : public single_threaded_countable_t<db_t> {
public:
    db_t(uuid_u _id, const std::string &_name) : id(_id), name(_name) { }
    const uuid_u id;
    const std::string name;
};

class table_t : public single_threaded_countable_t<table_t>, public pb_rcheckable_t {
public:
    table_t(env_t *env,
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
        counted_t<const datum_t> left_bound, bool left_bound_open,
        counted_t<const datum_t> right_bound, bool right_bound_open,
        const std::string &sindex_id, const rcheckable_t *parent);

    counted_t<const datum_t> make_error_datum(const base_exc_t &exception);


    counted_t<const datum_t> replace(env_t *env,
                                     counted_t<const datum_t> orig,
                                     counted_t<func_t> f,
                                     bool nondet_ok,
                                     durability_requirement_t durability_requirement,
                                     bool return_vals);
    counted_t<const datum_t> replace(env_t *env,
                                     counted_t<const datum_t> orig,
                                     counted_t<const datum_t> d,
                                     bool upsert,
                                     durability_requirement_t durability_requirement,
                                     bool return_vals);

    std::vector<counted_t<const datum_t> > batch_replace(
        env_t *env,
        const std::vector<counted_t<const datum_t> > &original_values,
        counted_t<func_t> replacement_generator,
        bool nondeterministic_replacements_ok,
        durability_requirement_t durability_requirement);

    std::vector<counted_t<const datum_t> > batch_replace(
        env_t *env,
        const std::vector<counted_t<const datum_t> > &original_values,
        const std::vector<counted_t<const datum_t> > &replacement_values,
        bool upsert,
        durability_requirement_t durability_requirement);

    MUST_USE bool sindex_create(env_t *env, const std::string &name,
            counted_t<func_t> index_func, sindex_multi_bool_t multi);
    MUST_USE bool sindex_drop(env_t *env, const std::string &name);
    counted_t<const datum_t> sindex_list(env_t *env);

    counted_t<const db_t> db;
    const std::string name;
private:
    struct datum_func_pair_t {
        datum_func_pair_t() : original_value(NULL), replacer(NULL), error_value(NULL) { }
        datum_func_pair_t(counted_t<const datum_t> _original_value,
                          const map_wire_func_t *_replacer)
            : original_value(_original_value), replacer(_replacer), error_value(NULL) { }

        explicit datum_func_pair_t(counted_t<const datum_t> _error_value)
            : original_value(NULL), replacer(NULL), error_value(_error_value) { }

        // One of these counted_t<const datum_t>'s is empty.
        counted_t<const datum_t> original_value;
        const map_wire_func_t *replacer;
        counted_t<const datum_t> error_value;
    };

    std::vector<counted_t<const datum_t> > batch_replace(
        env_t *env,
        const std::vector<datum_func_pair_t> &replacements,
        durability_requirement_t durability_requirement);

    counted_t<const datum_t> do_replace(env_t *env, counted_t<const datum_t> orig,
                                        const map_wire_func_t &mwf,
                                        durability_requirement_t durability_requirement,
                                        bool return_vals);
    counted_t<const datum_t> do_replace(env_t *env, counted_t<const datum_t> orig,
                                        counted_t<func_t> f,
                                        bool nondet_ok,
                                        durability_requirement_t durability_requirement,
                                        bool return_vals);
    counted_t<const datum_t> do_replace(env_t *env, counted_t<const datum_t> orig,
                                        counted_t<const datum_t> d,
                                        bool upsert,
                                        durability_requirement_t durability_requirement,
                                        bool return_vals);

    bool use_outdated;
    std::string pkey;
    scoped_ptr_t<namespace_repo_t<rdb_protocol_t>::access_t> access;

    boost::optional<std::string> sindex_id;
    sorting_t sorting;

    struct bound_t {
        bound_t(counted_t<const datum_t> _value, bool _bound_open)
            : value(_value), bound_open(_bound_open) { }
        counted_t<const datum_t> value;
        bool bound_open;
    };

    boost::optional<std::pair<bound_t, bound_t> > bounds;
};


enum function_shortcut_t {
    NO_SHORTCUT = 0,
    CONSTANT_SHORTCUT = 1,
    GET_FIELD_SHORTCUT = 2,
    PLUCK_SHORTCUT = 3
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
                        Response *res, stream_cache2_t *stream_cache2,
                        bool *response_needed_out);
    public:
        enum raw_type_t {
            DB               = 1, // db
            TABLE            = 2, // table
            SELECTION        = 3, // table, sequence
            SEQUENCE         = 4, // sequence
            SINGLE_SELECTION = 5, // table, datum (object)
            DATUM            = 6, // datum
            FUNC             = 7  // func
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
    val_t(counted_t<const datum_t> _datum, counted_t<table_t> _table, protob_t<const Backtrace> backtrace);
    val_t(env_t *env, counted_t<datum_stream_t> _sequence, protob_t<const Backtrace> backtrace);
    val_t(counted_t<table_t> _table, protob_t<const Backtrace> backtrace);
    val_t(counted_t<table_t> _table, counted_t<datum_stream_t> _sequence, protob_t<const Backtrace> backtrace);
    val_t(counted_t<const db_t> _db, protob_t<const Backtrace> backtrace);
    val_t(counted_t<func_t> _func, protob_t<const Backtrace> backtrace);
    ~val_t();

    counted_t<const db_t> as_db() const;
    counted_t<table_t> as_table();
    std::pair<counted_t<table_t> , counted_t<datum_stream_t> > as_selection(env_t *env);
    counted_t<datum_stream_t> as_seq(env_t *env);
    std::pair<counted_t<table_t> , counted_t<const datum_t> > as_single_selection();
    // See func.hpp for an explanation of shortcut functions.
    counted_t<func_t> as_func(function_shortcut_t shortcut = NO_SHORTCUT);

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
    const std::string &as_str();

    std::string print() const;
    std::string trunc_print() const;

private:
    void rcheck_literal_type(type_t::raw_type_t expected_raw_type) const;

    type_t type;
    counted_t<table_t> table;

    // We pretend that this variant is a union -- as if it doesn't have type
    // information.  The sequence, datum, func, and db_ptr functions get the
    // fields of the variant.
    boost::variant<counted_t<const db_t>,
                   counted_t<datum_stream_t>,
                   counted_t<const datum_t>,
                   counted_t<func_t> > u;

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
