// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef PROTOCOL_API_HPP_
#define PROTOCOL_API_HPP_

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "buffer_cache/types.hpp"
#include "clustering/administration/auth/user_context.hpp"
#include "clustering/administration/auth/permission_error.hpp"
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
#include "utils.hpp"
#include "version.hpp"

namespace auth {
class username_t;
}  // namespace auth

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
    virtual void read(auth::user_context_t const &user_context,
                      const read_t &,
                      read_response_t *response,
                      order_token_t tok,
                      signal_t *interruptor)
        THROWS_ONLY(
            interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t) = 0;

    virtual void write(auth::user_context_t const &user_context,
                       const write_t &,
                       write_response_t *response,
                       order_token_t tok,
                       signal_t *interruptor)
        THROWS_ONLY(
            interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t) = 0;

    /* These calls are for the sole purpose of optimizing queries; don't rely
    on them for correctness. They should not block. */
    virtual std::set<region_t> get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t) = 0;

    virtual signal_t *get_initial_ready_signal() { return nullptr; }

    virtual bool check_readiness(table_readiness_t readiness,
                                 signal_t *interruptor) = 0;

protected:
    virtual ~namespace_interface_t() { }
};

/* `namespace_interface_access_t` is like a smart pointer to a `namespace_interface_t`.
This is the format in which `real_table_t` expects to receive its
`namespace_interface_t *`. This allows the thing that is constructing the `real_table_t`
to control the lifetime of the `namespace_interface_t`, but also allows the
`real_table_t` to block it from being destroyed while in use. */
class namespace_interface_access_t {
public:
    class ref_tracker_t {
    public:
        virtual void add_ref() = 0;
        virtual void release() = 0;
    protected:
        virtual ~ref_tracker_t() { }
    };
    namespace_interface_access_t();
    namespace_interface_access_t(namespace_interface_t *, ref_tracker_t *, threadnum_t);
    namespace_interface_access_t(const namespace_interface_access_t &access);
    namespace_interface_access_t &operator=(const namespace_interface_access_t &access);
    ~namespace_interface_access_t();

    namespace_interface_t *get();

private:
    namespace_interface_t *nif;
    ref_tracker_t *ref_tracker;
    threadnum_t thread;
};

// Specifies the desired behavior for insert operations, upon discovering a
// conflict.
//  - conflict_behavior_t::ERROR: Signal an error upon conflicts.
//  - conflict_behavior_t::REPLACE: Replace the old row with the new row if a
//    conflict occurs.
//  - conflict_behavior_t::UPDATE: Merge the old and new rows if a conflict
//    occurs.
enum class conflict_behavior_t { ERROR, REPLACE, UPDATE, FUNCTION };

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(conflict_behavior_t,
                                      int8_t,
                                      conflict_behavior_t::ERROR,
                                      conflict_behavior_t::FUNCTION);

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

enum class emergency_repair_mode_t { DEBUG_RECOMMIT,
                                     UNSAFE_ROLLBACK,
                                     UNSAFE_ROLLBACK_OR_ERASE };

/* `backfill_item_memory_tracker_t` is used by the backfilling logic to control the
memory usage on the backfill sender. It is updated whenever a key/value pair is
loaded, or a new backfill_item_t structure is allocated. */
class backfill_item_memory_tracker_t {
public:
    explicit backfill_item_memory_tracker_t(size_t memory_limit)
        : remaining_memory(memory_limit), had_at_least_one_item(false) { }

    bool is_limit_exceeded() const {
        return had_at_least_one_item && remaining_memory < 0;
    }
    void reserve_memory(size_t mem_size) {
        remaining_memory -= mem_size;
    }
    void note_item() {
        had_at_least_one_item = true;
    }
private:
    ssize_t remaining_memory;

    /* We need to ensure that the backfill makes progress. If we have a key/value
    pair that was larger than the memory limit, we would get stuck if we enforced
    the memory limit strictly.
    Hence we always let the first item through. `had_at_least_one_item` is used
    to track whether this has already happened. */
    bool had_at_least_one_item;
};

#endif /* PROTOCOL_API_HPP_ */
