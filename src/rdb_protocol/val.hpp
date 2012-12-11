#ifndef RDB_PROTOCOL_VAL_HPP_
#define RDB_PROTOCOL_VAL_HPP_

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/stream.hpp" // TOOD: fix up

namespace ql {
typedef query_language::json_stream_t json_stream_t;
// namespace_repo_t<rdb_protocol_t>::access_t *
class table_t;
class func_t;

class val_t {
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

    val_t(const datum_t *datum);

    uuid_t as_db();
    table_t *as_table();
    std::pair<table_t *, json_stream_t *> as_selection();
    json_stream_t *as_seq();
    std::pair<table_t *, const datum_t *> as_single_selection();
    const datum_t *as_datum();
    func_t *as_func();
private:
    type_t type;
    uuid_t db;
    table_t *table;
    scoped_ptr_t<json_stream_t> sequence;
    scoped_ptr_t<const datum_t> datum;
    scoped_ptr_t<func_t> func;

    bool consumed;
};
} //namespace ql
#endif // RDB_PROTOCOL_VAL_HPP_
