#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/func.hpp"
#include "rdb_protocol/ql2.pb.h"
#include "rdb_protocol/stream.hpp" // TOOD: fix up

namespace ql {
class env_t;
class val_t;
class term_t;
// namespace_repo_t<rdb_protocol_t>::access_t *

class datum_stream_t : public ptr_baggable_t {
public:
    datum_stream_t(env_t *_env);
    virtual ~datum_stream_t() { }
    virtual const datum_t *next() = 0;
    virtual datum_stream_t *map(func_t *f);
protected:
    env_t *env;
};

class lazy_datum_stream_t : public datum_stream_t {
public:
    lazy_datum_stream_t(env_t *env, bool use_outdated,
                        namespace_repo_t<rdb_protocol_t>::access_t *ns_access);
    virtual datum_stream_t *map(func_t *f);
    virtual const datum_t *next();
private:
    lazy_datum_stream_t(lazy_datum_stream_t *src);
    boost::shared_ptr<query_language::json_stream_t> json_stream;

    rdb_protocol_details::transform_variant_t trans;
    query_language::scopes_t _s;
    query_language::backtrace_t _b;
};

class array_datum_stream_t : public datum_stream_t {
public:
    array_datum_stream_t(env_t *env, const datum_t *_arr);
    virtual const datum_t *next();
private:
    size_t index;
    const datum_t *arr;
};

class map_datum_stream_t : public datum_stream_t {
public:
    map_datum_stream_t(env_t *env, func_t *_f, datum_stream_t *_src);
    virtual const datum_t *next();
private:
    func_t *f;
    datum_stream_t *src;
};

class table_t : public ptr_baggable_t {
public:
    table_t(env_t *_env, uuid_t db_id, const std::string &name, bool use_outdated);
    datum_stream_t *as_datum_stream();
    const std::string &get_pkey();
    const datum_t *get_row(const datum_t *pval);
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
        const char *name() const;
        raw_type_t raw_type;
    };
    type_t get_type() const;

    val_t(const datum_t *_datum, const term_t *_parent, env_t *_env);
    val_t(const datum_t *_datum, table_t *_table, const term_t *_parent, env_t *_env);
    val_t(datum_stream_t *_sequence, const term_t *_parent, env_t *_env);
    val_t(table_t *_table, const term_t *_parent, env_t *_env);
    val_t(uuid_t _db, const term_t *_parent, env_t *_env);
    val_t(func_t *_func, const term_t *_parent, env_t *_env);

    uuid_t as_db(); // X
    table_t *as_table(); // X
    std::pair<table_t *, datum_stream_t *> as_selection();
    datum_stream_t *as_seq(); // X
    std::pair<table_t *, const datum_t *> as_single_selection(); // X
    const datum_t *as_datum(); // X
    func_t *as_func(); // X
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

uuid_t get_db_uuid(env_t *env, const std::string &dbs);

} //namespace ql
#endif // RDB_PROTOCOL_VAL_HPP_
