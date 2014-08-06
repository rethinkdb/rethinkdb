// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_
#define RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_

#include <string>

#include "concurrency/pubsub.hpp"
#include "rdb_protocol/datum.hpp"

class artificial_table_backend_t : public home_thread_mixin_t {
public:
    /* Notes:
     1. If `set_row()` is called concurrently with `get_row()` or
        `get_all_primary_keys()`, it is undefined whether the read will see the write or
        not.
     2. `get_primary_key_name()`, `readd_all_primary_keys()`, `read_row()` and
        `write_row()` can be called on any thread. Beware of problems when transferring
        `counted_t<const ql::datum_t>` between threads!
     3. `get_publisher()` can only be called on the home thread. The return value will
        always deliver notifications on that same thread.
    */

    /* Returns the name of the primary key for the table. The return value must not
    change. This must not block. */
    virtual std::string get_primary_key_name() = 0;

    /* Returns the primary keys of all of the rows that exist. */
    virtual bool read_all_primary_keys(
        signal_t *interruptor,
        std::vector<counted_t<const ql::datum_t> > *keys_out,
        std::string *error_out) = 0;

    /* Returns the current value of the row, or an empty `counted_t` if no such row
    exists. */
    virtual bool read_row(
        counted_t<const ql::datum_t> primary_key,
        signal_t *interruptor,
        counted_t<const ql::datum_t> *row_out,
        std::string *error_out) = 0;

    /* Called when the user issues a write command on the row. Calling `set_row()` on a
    row that doesn't exist means an insertion; calling `set_row` with `new_value` an
    empty `counted_t` means a deletion. */
    virtual bool write_row(
        counted_t<const ql::datum_t> primary_key,
        counted_t<const ql::datum_t> new_value,
        signal_t *interruptor,
        std::string *error_out) = 0;

    /* TODO: Support change feeds. */

protected:
    virtual ~artificial_table_backend_t() { }
};

#endif /* RDB_PROTOCOL_ARTIFICIAL_TABLE_BACKEND_HPP_ */

