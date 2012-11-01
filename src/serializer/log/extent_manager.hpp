// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_LOG_EXTENT_MANAGER_HPP_
#define SERIALIZER_LOG_EXTENT_MANAGER_HPP_

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <deque>

#include "utils.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

#include "arch/types.hpp"
#include "config/args.hpp"
#include "containers/scoped.hpp"
#include "serializer/log/config.hpp"
#include "containers/segmented_vector.hpp"

#define NULL_OFFSET off64_t(-1)

class extent_zone_t;

struct log_serializer_stats_t;

class extent_transaction_t {
public:
    friend class extent_manager_t;
    extent_transaction_t() : active_(false) { }
    ~extent_transaction_t() {
        rassert(!active_);
    }

    void init() { active_ = true; }
    void reset() {
        free_queue_.clear();
        active_ = false;
    }

    std::deque<off64_t> &free_queue() {
        rassert(active_);
        return free_queue_;
    }

private:
    bool active_;
    std::deque<off64_t> free_queue_;

    DISABLE_COPYING(extent_transaction_t);
};

class extent_manager_t : public home_thread_mixin_debug_only_t {
public:
    struct metablock_mixin_t {
        int64_t padding;
    };

    extent_manager_t(direct_file_t *file, const log_serializer_on_disk_static_config_t *static_config, const log_serializer_dynamic_config_t *dynamic_config, log_serializer_stats_t *);
    ~extent_manager_t();

    /* When we load a database, we use reserve_extent() to inform the extent manager
    which extents were already in use */

    void reserve_extent(off64_t extent);

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

    void begin_transaction(extent_transaction_t *out);
    off64_t gen_extent();
    void release_extent(off64_t extent, extent_transaction_t *txn);
    void end_transaction(extent_transaction_t *t);
    void commit_transaction(extent_transaction_t *t);

    /* Number of extents that have been released but not handed back out again. */
    int held_extents();

    log_serializer_stats_t *const stats;
    const uint64_t extent_size;   /* Same as static_config->extent_size */

private:
    extent_zone_t *zone_for_offset(off64_t offset);

    const log_serializer_on_disk_static_config_t *const static_config;
    const log_serializer_dynamic_config_t *const dynamic_config;

    boost::ptr_vector<extent_zone_t> zones;
    int next_zone;    /* Which zone to give the next extent from */

    direct_file_t *const dbfile;

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
