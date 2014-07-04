// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_EXTENT_MANAGER_HPP_
#define SERIALIZER_LOG_EXTENT_MANAGER_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <utility>
#include <vector>

#include "arch/types.hpp"
#include "config/args.hpp"
#include "containers/scoped.hpp"
#include "serializer/log/config.hpp"

#define NULL_OFFSET int64_t(-1)

class extent_zone_t;

struct log_serializer_stats_t;

// A reference to an extent in the extent manager.  An extent may not be freed until
// all of the references go away (unless the server is shutting down).
class extent_reference_t {
public:
    extent_reference_t() : extent_offset_(-1) {}
    explicit extent_reference_t(int64_t extent_offset) : extent_offset_(extent_offset) {}
    extent_reference_t(extent_reference_t &&movee)
        : extent_offset_(movee.release()) {}
    ~extent_reference_t() { guarantee(extent_offset_ == -1); }

    void operator=(extent_reference_t &&movee) {
        extent_reference_t tmp(std::move(movee));
        std::swap(extent_offset_, tmp.extent_offset_);
    }

    void init(int64_t offset) {
        guarantee(extent_offset_ == -1);
        extent_offset_ = offset;
    }

    int64_t offset() const {
        guarantee(extent_offset_ != -1);
        return extent_offset_;
    }

    MUST_USE int64_t release() {
        guarantee(extent_offset_ != -1);
        int64_t ret = extent_offset_;
        extent_offset_ = -1;
        return ret;
    }

private:
    int64_t extent_offset_;
    DISABLE_COPYING(extent_reference_t);
};

class extent_transaction_t {
public:
    friend class extent_manager_t;
    extent_transaction_t() : state_(uninitialized) { }
    ~extent_transaction_t() {
        rassert(state_ == committed);
    }

    void init() {
        guarantee(state_ == uninitialized);
        state_ = begun;
    }
    void push_extent(extent_reference_t &&extent_ref) {
        guarantee(state_ == begun);
        extent_ref_set_.push_back(std::move(extent_ref));
    }
    void mark_end() {
        guarantee(state_ == begun);
        state_ = ended;
    }
    MUST_USE std::vector<extent_reference_t> reset() {
        guarantee(state_ == ended);
        state_ = committed;
        return std::move(extent_ref_set_);
    }

private:
    enum { uninitialized, begun, ended, committed } state_;
    std::vector<extent_reference_t> extent_ref_set_;

    DISABLE_COPYING(extent_transaction_t);
};

class extent_manager_t : public home_thread_mixin_debug_only_t {
public:
    struct metablock_mixin_t {
        int64_t padding;
    };

    extent_manager_t(file_t *file,
                     const log_serializer_on_disk_static_config_t *static_config,
                     log_serializer_stats_t *);
    ~extent_manager_t();

    /* When we load a database, we use reserve_extent() to inform the extent manager
    which extents were already in use */

    MUST_USE extent_reference_t reserve_extent(int64_t extent);

    static void prepare_initial_metablock(metablock_mixin_t *mb);
    void start_existing(metablock_mixin_t *last_metablock);
    void prepare_metablock(metablock_mixin_t *metablock);
    void shutdown();

    /* The extent manager uses transactions to make sure that extents are not freed
    before it is safe to free them. An extent manager transaction is created for every
    log serializer write transaction. Any extents that are freed in the course of
    perfoming the write transaction are recorded in a "holding area" on the extent
    manager transaction. They are only allowed to be reused after the transaction
    is committed; the log serializer only commits the transaction after the metablock
    has been written. This guarantees that we will not overwrite extents that the
    most recent metablock points to. */

    MUST_USE extent_reference_t copy_extent_reference(const extent_reference_t &copyee);

    void begin_transaction(extent_transaction_t *out);
    MUST_USE extent_reference_t gen_extent();
    void release_extent_into_transaction(extent_reference_t &&extent_ref,
                                         extent_transaction_t *txn);
    void release_extent(extent_reference_t &&extent_ref);
    void end_transaction(extent_transaction_t *t);
    void commit_transaction(extent_transaction_t *t);

    /* Number of extents that have been released but not handed back out again. */
    size_t held_extents();

    log_serializer_stats_t *const stats;
    const uint64_t extent_size;   /* Same as static_config->extent_size */

private:
    void release_extent_preliminaries();

    scoped_ptr_t<extent_zone_t> zone;

    /* During serializer startup, each component informs the extent manager
    which extents in the file it was using at shutdown. This is the
    "state_reserving_extents" phase. Then extent_manager_t::start() is called
    and it is in "state_running", where it releases and generates extents. */
    enum state_t {
        state_reserving_extents,
        state_running,
        state_shut_down
    } state;

    extent_transaction_t *current_transaction;

    DISABLE_COPYING(extent_manager_t);
};
#endif /* SERIALIZER_LOG_EXTENT_MANAGER_HPP_ */
