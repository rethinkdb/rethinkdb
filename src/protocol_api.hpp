// Copyright 2010-2014 RethinkDB, all rights reserved.
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
#include "containers/scoped.hpp"
#include "containers/object_buffer.hpp"
#include "region/region.hpp"
#include "region/region_map.hpp"
#include "rpc/serialize_macros.hpp"
#include "timestamps.hpp"
#include "version.hpp"

struct backfill_chunk_t;
struct read_t;
struct read_response_t;
class store_t;
class store_view_t;
class traversal_progress_combiner_t;
struct write_t;
struct write_response_t;

class cannot_perform_query_exc_t : public std::exception {
public:
    explicit cannot_perform_query_exc_t(const std::string &s) : message(s) { }
    ~cannot_perform_query_exc_t() throw () { }
    const char *what() const throw () {
        return message.c_str();
    }
private:
    std::string message;
};

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
    virtual void read_outdated(const read_t &,
                               read_response_t *response,
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

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        reql_version_t, int8_t,
        reql_version_t::EARLIEST, reql_version_t::LATEST);

#endif /* PROTOCOL_API_HPP_ */
