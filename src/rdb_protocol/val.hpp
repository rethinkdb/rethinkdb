#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/datum_stream.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream.hpp"

namespace ql {
class env_t;
class val_t;
class term_t;
// namespace_repo_t<rdb_protocol_t>::access_t *

class table_t : public ptr_baggable_t {
public:
    table_t(env_t *_env, uuid_t db_id, const std::string &name, bool use_outdated);
    datum_stream_t *as_datum_stream();
    const std::string &get_pkey();
    const datum_t *get_row(const datum_t *pval);

    const datum_t *replace(const datum_t *orig, const map_wire_func_t &mwf);
    const datum_t *replace(const datum_t *orig, func_t *f);
    const datum_t *replace(const datum_t *orig, const datum_t *d, bool upsert);
private:
    env_t *env;
    bool use_outdated;
    std::string pkey;
    scoped_ptr_t<namespace_repo_t<rdb_protocol_t>::access_t> access;
};

class val_t : public ptr_baggable_t {
public:
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
        type_t(raw_type_t _raw_type);
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
    val_t(uuid_t _db, const term_t *_parent, env_t *_env);
    val_t(func_t *_func, const term_t *_parent, env_t *_env);

    uuid_t as_db();
    table_t *as_table();
    std::pair<table_t *, datum_stream_t *> as_selection();
    datum_stream_t *as_seq();
    std::pair<table_t *, const datum_t *> as_single_selection();
    const datum_t *as_datum();
    func_t *as_func();

    std::string print() {
        if (get_type().is_convertible(type_t::DATUM)) {
            return as_datum()->print();
        } else {
            return strprintf("OPAQUE VAL %s", get_type().name());
        }
    }
private:
    const term_t *parent;
    env_t *env;

    type_t type;
    uuid_t db;
    table_t *table;
    datum_stream_t *sequence;
    const datum_t *datum;
    func_t *func;

    bool consumed;
};

} //namespace ql
#endif // RDB_PROTOCOL_VAL_HPP_
