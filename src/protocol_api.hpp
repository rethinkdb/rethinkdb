// Copyright 2010-2014 RethinkDB, all rights reserved.

/// @file protocol_api.hpp
/// @brief Protocol-agnostic database interface for query routing
///
/// Defines the abstract interfaces for database operations (reads/writes),
/// namespaces, and store views. This abstraction allows different query
/// protocols to interact with the same underlying database implementation.
///
/// @defgroup ProtocolAPI Protocol API and Database Interfaces
/// Interfaces for protocol-independent database access
/// @{

#ifndef PROTOCOL_API_HPP_
#define PROTOCOL_API_HPP_

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
#include "utils.hpp"
#include "version.hpp"

namespace auth {
class permission_error_t;
class user_context_t;
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

/// @brief Exception thrown when a query cannot be performed
///
/// Indicates that a query (read or write) could not be executed.
/// Includes a human-readable error message and indicates whether the
/// query state is FAILED (error occurred) or INDETERMINATE (unclear).
///
/// @example
/// @code
/// try {
///     namespace_interface_t* ns = get_namespace_interface();
///     // Attempt read operation...
/// } catch (const cannot_perform_query_exc_t &e) {
///     if (e.get_query_state() == query_state_t::FAILED) {
///         std::cerr << "Query definitely failed: " << e.what() << std::endl;
///     } else {
///         std::cerr << "Query state unknown: " << e.what() << std::endl;
///     }
/// }
/// @endcode
class cannot_perform_query_exc_t : public std::exception {
public:
    /// @brief Default constructor for deserialization
    // SHOULD ONLY BE USED FOR SERIALIZATION
    cannot_perform_query_exc_t()
        : message("UNINITIALIZED"), query_state(query_state_t::FAILED) { }

    /// @brief Constructs an exception with message and state
    /// @param s The error message
    /// @param _query_state Whether the query FAILED or is INDETERMINATE
    cannot_perform_query_exc_t(const std::string &s, query_state_t _query_state)
        : message(s), query_state(_query_state) { }

    /// @brief Returns the error message
    const char *what() const throw () {
        return message.c_str();
    }

    /// @brief Returns the query state
    query_state_t get_query_state() const throw () { return query_state; }

private:
    RDB_DECLARE_ME_SERIALIZABLE(cannot_perform_query_exc_t);
    std::string message;
    query_state_t query_state;
};
RDB_DECLARE_SERIALIZABLE_FOR_CLUSTER(cannot_perform_query_exc_t);

/// @brief Enumeration for table readiness levels
///
/// Indicates the current readiness state of a table for serving different
/// types of operations.
///
/// @example
/// @code
/// if (namespace_interface->check_readiness(table_readiness_t::writes, &signal)) {
///     // Table is ready for write operations
/// }
/// @endcode
enum class table_readiness_t {
    unavailable,          ///< Table is not available
    outdated_reads,       ///< Can perform reads but data may be stale
    reads,                ///< Can perform reads with consistent data
    writes,               ///< Can perform both reads and writes
    finished              ///< Table is fully ready
};

/// @brief Protocol-agnostic interface for database operations
///
/// Abstract interface that provides protocol-independent access to database
/// operations. Different query protocols (ReQL, SQL, etc.) use this interface
/// to interact with the underlying database.
///
/// @example
/// @code
/// class namespace_interface_t {
///     virtual void read(auth::user_context_t const &user_context,
///                       const read_t &read,
///                       read_response_t *response,
///                       order_token_t tok,
///                       signal_t *interruptor)
///         THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t, 
///                    auth::permission_error_t) = 0;
///
///     virtual void write(auth::user_context_t const &user_context,
///                        const write_t &write,
///                        write_response_t *response,
///                        order_token_t tok,
///                        signal_t *interruptor)
///         THROWS_ONLY(interrupted_exc_t, cannot_perform_query_exc_t,
///                    auth::permission_error_t) = 0;
/// };
/// @endcode
class namespace_interface_t {
public:
    /// @brief Performs a read operation on the database
    /// @param user_context Authentication context for the user
    /// @param read The read operation specification
    /// @param response Output parameter to receive the read result
    /// @param tok Order token for maintaining operation ordering
    /// @param interruptor Signal to interrupt the operation
    /// @throws interrupted_exc_t if the interruptor signal fires
    /// @throws cannot_perform_query_exc_t if the read cannot be performed
    /// @throws auth::permission_error_t if the user lacks permission
    virtual void read(auth::user_context_t const &user_context,
                      const read_t &,
                      read_response_t *response,
                      order_token_t tok,
                      signal_t *interruptor)
        THROWS_ONLY(
            interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t) = 0;

    /// @brief Performs a write operation on the database
    /// @param user_context Authentication context for the user
    /// @param write The write operation specification
    /// @param response Output parameter to receive the write result
    /// @param tok Order token for maintaining operation ordering
    /// @param interruptor Signal to interrupt the operation
    /// @throws interrupted_exc_t if the interruptor signal fires
    /// @throws cannot_perform_query_exc_t if the write cannot be performed
    /// @throws auth::permission_error_t if the user lacks permission
    virtual void write(auth::user_context_t const &user_context,
                       const write_t &,
                       write_response_t *response,
                       order_token_t tok,
                       signal_t *interruptor)
        THROWS_ONLY(
            interrupted_exc_t, cannot_perform_query_exc_t, auth::permission_error_t) = 0;

    /// @brief Gets the sharding scheme of the database
    /// Returns the regions into which the database is partitioned.
    /// This is an optimization hint for query routing - do not rely on it for correctness.
    /// This function should not block.
    /// @return A set of regions representing the database sharding scheme
    /// @throws cannot_perform_query_exc_t if the scheme cannot be determined
    virtual std::set<region_t> get_sharding_scheme()
        THROWS_ONLY(cannot_perform_query_exc_t) = 0;

    /// @brief Gets a signal that fires when the table becomes initially ready
    /// Optional override for implementations that track readiness explicitly.
    /// @return A signal_t that fires when the table is ready, or nullptr
    virtual signal_t *get_initial_ready_signal() { return nullptr; }

    /// @brief Checks if the table meets a readiness requirement
    /// Determines if the table is ready for a specific level of operations.
    /// @param readiness The required readiness level
    /// @param interruptor Signal to interrupt the check
    /// @return true if the table meets the readiness requirement
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

// Specifies whether or not to ignore a write hook on a table while doing an
// insert or a replace.
//  - ignore_write_hook_t::YES: Ignores the write hook, requires config permissions.
//  - ignore_write_hook_t::NO: Applies the write hook as normal.
enum class ignore_write_hook_t {
    NO = 0,
    YES = 1
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(ignore_write_hook_t,
                                      int8_t,
                                      ignore_write_hook_t::NO,
                                      ignore_write_hook_t::YES);

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
