// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PROTOCOL_API_HPP_
#define PROTOCOL_API_HPP_

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "utils.hpp"

#include "buffer_cache/types.hpp"
#include "concurrency/fifo_checker.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/signal.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/binary_blob.hpp"
#include "containers/object_buffer.hpp"
#include "containers/scoped.hpp"
#include "region/region.hpp"
#include "region/region_map.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "version.hpp"

class backfill_item_t;
struct backfill_chunk_t;
struct read_t;
struct read_response_t;
class store_t;
class store_view_t;
class traversal_progress_combiner_t;
struct write_t;
struct write_response_t;

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        query_state_t, int8_t, query_state_t::FAILED, query_state_t::INDETERMINATE);
class cannot_perform_query_exc_t : public std::exception {
public:
    // SHOULD ONLY BE USED FOR SERIALIZATION
    cannot_perform_query_exc_t()
        : message("UNINITIALIZED"), query_state(query_state_t::FAILED) { }
    cannot_perform_query_exc_t(const std::string &s, query_state_t _query_state)
        : message(s), query_state(_query_state) { }
    ~cannot_perform_query_exc_t() throw () { }
    const char *what() const throw () {
        return message.c_str();
    }
    query_state_t get_query_state() const throw () { return query_state; }
private:
    RDB_DECLARE_ME_SERIALIZABLE(cannot_perform_query_exc_t);
    std::string message;
    query_state_t query_state;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(cannot_perform_query_exc_t);

enum class table_readiness_t {
    unavailable,
    outdated_reads,
    reads,
    writes,
    finished
};

/* `namespace_interface_t` is the interface that the protocol-agnostic database
logic for query routing exposes to the protocol-specific query parser. */

class namespace_interface_t {
public:
    virtual void read(const read_t &,
                      read_response_t *response,
                      order_token_t tok,
                      signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) = 0;
    virtual void write(const write_t &,
                       write_response_t *response,
                       order_token_t tok,
                       signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t) = 0;

    /* These calls are for the sole purpose of optimizing queries; don't rely
    on them for correctness. They should not block. */
    virtual std::set<region_t> get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t) = 0;

    virtual signal_t *get_initial_ready_signal() { return NULL; }

    virtual bool check_readiness(table_readiness_t readiness,
                                 signal_t *interruptor) = 0;

protected:
    virtual ~namespace_interface_t() { }
};

// Specifies the desired behavior for insert operations, upon discovering a
// conflict.
//  - conflict_behavior_t::ERROR: Signal an error upon conflicts.
//  - conflict_behavior_t::REPLACE: Replace the old row with the new row if a
//    conflict occurs.
//  - conflict_behavior_t::UPDATE: Merge the old and new rows if a conflict
//    occurs.
enum class conflict_behavior_t { ERROR, REPLACE, UPDATE };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(conflict_behavior_t,
                                      int8_t,
                                      conflict_behavior_t::ERROR,
                                      conflict_behavior_t::UPDATE);

// Specifies the durability requirements of a write operation.
//  - DURABILITY_REQUIREMENT_DEFAULT: Use the table's durability settings.
//  - DURABILITY_REQUIREMENT_HARD: Override the table's durability settings with
//    hard durability.
//  - DURABILITY_REQUIREMENT_SOFT: Override the table's durability settings with
//    soft durability.
enum durability_requirement_t { DURABILITY_REQUIREMENT_DEFAULT,
                                DURABILITY_REQUIREMENT_HARD,
                                DURABILITY_REQUIREMENT_SOFT };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(durability_requirement_t,
                                      int8_t,
                                      DURABILITY_REQUIREMENT_DEFAULT,
                                      DURABILITY_REQUIREMENT_SOFT);

enum class read_mode_t { MAJORITY, SINGLE, OUTDATED, DEBUG_DIRECT };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(read_mode_t,
                                      int8_t,
                                      read_mode_t::MAJORITY,
                                      read_mode_t::DEBUG_DIRECT);

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        reql_version_t, int8_t,
        reql_version_t::EARLIEST, reql_version_t::LATEST);

/* `store_backfill_item_consumer_t` is one type of callbacks used by
`store_view_t::send_backfill()`.

The semantics of `on_item()` and `on_empty_range()` are the same as for
`store_view_t::send_backfill_pre()`. The only difference is that these functions
don't return a `continue_bool_t`. This is because enforcing a memory limit is handled
separately through the `remaining_memory` and `had_at_least_one_item` fields.
The metainfo blob that is passed to `on_item()` and `on_empty_range()` is
guaranteed to cover at least the region from the right-hand edge of the previous
item to the right-hand edge of the current item; it may or may not cover a
larger area as well.

This could be a type inside of `store_view_t` in store_view.hpp like the other
similar types, but we need to access this in `btree/backfill.hpp` and don't want
to include the store_view.hpp header from the `rdb_protocol` directory there. */
class store_backfill_item_consumer_t {
public:
    store_backfill_item_consumer_t(ssize_t memory_limit)
        : remaining_memory(memory_limit), had_at_least_one_item(false) { }

    /* It's OK for `on_item()` and `on_empty_range()` to block, but they shouldn't
    block for very long, because the caller may hold B-tree locks while calling them.
    */
    virtual void on_item(
        const region_map_t<binary_blob_t> &metainfo,
        backfill_item_t &&item) THROWS_NOTHING = 0;
    virtual void on_empty_range(
        const region_map_t<binary_blob_t> &metainfo,
        const key_range_t::right_bound_t &threshold) THROWS_NOTHING = 0;

    /* These public fields are used by the backfilling logic to control the
    memory usage on the backfill sender. They are updated whenever a
    key/value pair is loaded, or a new backfill_item_t structure is allocated. */
    ssize_t remaining_memory;
    bool had_at_least_one_item;

protected:
    virtual ~store_backfill_item_consumer_t() { }
};

#endif /* PROTOCOL_API_HPP_ */
