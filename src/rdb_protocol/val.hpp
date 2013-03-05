#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include <string>
#include <utility>

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream.hpp"

namespace ql {
class env_t;
class val_t;
class term_t;

class table_t : public ptr_baggable_t, public pb_rcheckable_t {
public:
    table_t(env_t *_env, uuid_u db_id, const std::string &name,
            bool use_outdated, const pb_rcheckable_t *src);
    datum_stream_t *as_datum_stream();
    const std::string &get_pkey();
    const datum_t *get_row(const datum_t *pval);
    datum_t *env_add_ptr(datum_t *d);

    // A wrapper around `do_replace` that does error handling correctly.
    // TODO: Use a variadic template so we can get rid of
    // `_so_the_template_matches` above?
    template<class T>
    const datum_t *replace(const datum_t *d, T t, bool b) {
        rcheck(!use_outdated, "Cannot perform write operations on outdated tables.");
        try {
            return do_replace(d, t, b);
        } catch (const any_ql_exc_t &e) {
            datum_t *datum = env_add_ptr(new datum_t(datum_t::R_OBJECT));
            std::string err = e.what();
            // TODO why is this bool (which is marked as MUST USE not used?)
            // T0D0NE: the bool is true if there's a conflict when inserting the
            // key, but since we just created an empty object above conflicts
            // are impossible here.  If you want to harden this against future
            // changes, you could store the bool and `r_sanity_check` that it's
            // false.
            UNUSED bool key_in_object =
                datum->add("first_error", env_add_ptr(new datum_t(err)))
                || datum->add("errors", env_add_ptr(new datum_t(1L)));
            return datum;
        }
    }
private:
    const datum_t *do_replace(const datum_t *orig, const map_wire_func_t &mwf,
                             bool _so_the_template_matches = false);
    const datum_t *do_replace(const datum_t *orig, func_t *f, bool nondet_ok);
    const datum_t *do_replace(const datum_t *orig, const datum_t *d, bool upsert);

    env_t *env;
    bool use_outdated;
    std::string pkey;
    scoped_ptr_t<namespace_repo_t<rdb_protocol_t>::access_t> access;
};


enum function_shortcut_t {
    NO_SHORTCUT = 0,
    FILTER_SHORTCUT = 1,
    IDENTITY_SHORTCUT = 2
};

// A value is anything RQL can pass around -- a datum, a sequence, a function, a
// selection, whatever.
class val_t : public ptr_baggable_t, public pb_rcheckable_t {
public:
    // This type is intentionally opaque.  It is almost always an error to
    // compare two `val_t` types rather than testing whether one is convertible
    // to another.
    class type_t {
        friend class val_t;
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
    private:
        friend class coerce_term_t;
        friend class typeof_term_t;
        const char *name() const;
        raw_type_t raw_type;
    };
    type_t get_type() const;

    val_t(const datum_t *_datum, const term_t *_parent, env_t *_env);
    val_t(const datum_t *_datum, table_t *_table, const term_t *_parent, env_t *_env);
    val_t(datum_stream_t *_sequence, const term_t *_parent, env_t *_env);
    val_t(table_t *_table, const term_t *_parent, env_t *_env);
    val_t(table_t *_table, datum_stream_t *_sequence,
          const term_t *_parent, env_t *_env);
    val_t(uuid_u _db, const term_t *_parent, env_t *_env);
    val_t(func_t *_func, const term_t *_parent, env_t *_env);

    uuid_u as_db();
    table_t *as_table();
    std::pair<table_t *, datum_stream_t *> as_selection();
    datum_stream_t *as_seq();
    std::pair<table_t *, const datum_t *> as_single_selection();
    // See func.hpp for an explanation of shortcut functions.
    func_t *as_func(function_shortcut_t shortcut = NO_SHORTCUT);

    const datum_t *as_datum(); // prefer the 4 below
    bool as_bool();
    double as_num();
    template<class T>
    T as_int() {
        int64_t i = as_int();
        T t = static_cast<T>(i);
        rcheck(static_cast<int64_t>(t) == i,
               strprintf("Integer too large: %" PRIi64, i));
        return t;
    }
    int64_t as_int();
    const std::string &as_str();

    std::string print() {
        if (get_type().is_convertible(type_t::DATUM)) {
            return as_datum()->print();
        } else {
            // TODO: Do something smarter here?
            return strprintf("OPAQUE VAL %s", get_type().name());
        }
    }

private:
    void rcheck_literal_type(type_t::raw_type_t expected_raw_type);

    const term_t *parent;
    env_t *env;

    type_t type;
    uuid_u db;
    table_t *table;
    datum_stream_t *sequence;
    const datum_t *datum;
    func_t *func;

    DISABLE_COPYING(val_t);
};

} //namespace ql
#endif // RDB_PROTOCOL_VAL_HPP_
