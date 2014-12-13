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


#ifndef NDEBUG
// Checks that the metainfo has a certain value, or certain kind of value.
class metainfo_checker_callback_t {
public:
    virtual void check_metainfo(const region_map_t<binary_blob_t>& metainfo,
                                const region_t& domain) const = 0;
protected:
    metainfo_checker_callback_t() { }
    virtual ~metainfo_checker_callback_t() { }
private:
    DISABLE_COPYING(metainfo_checker_callback_t);
};


struct trivial_metainfo_checker_callback_t : public metainfo_checker_callback_t {

    trivial_metainfo_checker_callback_t() { }
    void check_metainfo(UNUSED const region_map_t<binary_blob_t>& metainfo, UNUSED const region_t& region) const {
        /* do nothing */
    }

private:
    DISABLE_COPYING(trivial_metainfo_checker_callback_t);
};

class metainfo_checker_t {
public:
    metainfo_checker_t(const metainfo_checker_callback_t *callback,
                       const region_t& region) : callback_(callback), region_(region) { }

    void check_metainfo(const region_map_t<binary_blob_t>& metainfo) const {
        callback_->check_metainfo(metainfo, region_);
    }
    const region_t& get_domain() const { return region_; }
    const metainfo_checker_t mask(const region_t& region) const {
        return metainfo_checker_t(callback_, region_intersection(region, region_));
    }

private:
    const metainfo_checker_callback_t *const callback_;
    const region_t region_;

    // This _is_ copyable because of mask, but all copies' lifetimes
    // are limited by that of callback_.
};

#endif  // NDEBUG

class chunk_fun_callback_t {
public:
    virtual void send_chunk(const backfill_chunk_t &, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) = 0;

protected:
    chunk_fun_callback_t() { }
    virtual ~chunk_fun_callback_t() { }
private:
    DISABLE_COPYING(chunk_fun_callback_t);
};

class send_backfill_callback_t : public chunk_fun_callback_t {
public:
    bool should_backfill(const region_map_t<binary_blob_t> &metainfo) {
        guarantee(!should_backfill_was_called_);
        should_backfill_was_called_ = true;
        return should_backfill_impl(metainfo);
    }

protected:
    virtual bool should_backfill_impl(const region_map_t<binary_blob_t> &metainfo) = 0;

    send_backfill_callback_t() : should_backfill_was_called_(false) { }
    virtual ~send_backfill_callback_t() { }
private:
    bool should_backfill_was_called_;

    DISABLE_COPYING(send_backfill_callback_t);
};

/* {read,write}_token_t hold the lock held when getting in line for the
   superblock. */
struct read_token_t {
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> main_read_token;
};

struct write_token_t {
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t> main_write_token;
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
        reql_version_t::v1_13, reql_version_t::v1_16_is_latest);

#endif /* PROTOCOL_API_HPP_ */
