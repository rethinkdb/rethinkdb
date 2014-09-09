// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_

#include <string>
#include <vector>

#include "rdb_protocol/datum.hpp"

/* `artificial_table_backend_t` is the interface that `artificial_table_t` uses to access
the actual data or configuration. There is one subclass for each table like
`rethinkdb.table_config`, `rethinkdb.table_status`, and so on. */

class artificial_table_backend_t : public home_thread_mixin_t {
public:
    /* Notes:
     1. `read_all_primary_keys()`, `read_row()`, and `write_row()` all return `false` and
        set `*error_out` if an error occurs. Note that if a row is absent in
        `read_row()`, this doesn't count as an error.
     2. If `write_row()` is called concurrently with `read_row()` or
        `read_all_primary_keys()`, it is undefined whether the read will see the write or
        not.
     3. `get_primary_key_name()`, `read_all_primary_keys()`, `read_row()` and
        `write_row()` can be called on any thread. */

    /* Returns the name of the primary key for the table. The return value must not
    change. This must not block. */
    virtual std::string get_primary_key_name() = 0;

    /* Sets `*keys_out` to the primary keys of all of the rows that exist. */
    virtual bool read_all_primary_keys(
        signal_t *interruptor,
        std::vector<ql::datum_t> *keys_out,
        std::string *error_out) = 0;

    /* Sets `*row_out` to the current value of the row, or an empty `datum_t` if no such
    row exists. */
    virtual bool read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        std::string *error_out) = 0;

    /* Called when the user issues a write command on the row. Calling `write_row()` on a
    row that doesn't exist means an insertion; calling `write_row` with `new_value` an
    empty `datum_t` means a deletion. */
    virtual bool write_row(
        ql::datum_t primary_key,
        ql::datum_t new_value,
        signal_t *interruptor,
        std::string *error_out) = 0;

    /* RSI(reql_admin): Support change feeds. */

protected:
    virtual ~artificial_table_backend_t() { }
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_ */

