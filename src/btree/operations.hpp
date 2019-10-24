// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_OPERATIONS_HPP_
#define BTREE_OPERATIONS_HPP_

#include <algorithm>
#include <utility>
#include <vector>

#include "btree/node.hpp"
#include "btree/stats.hpp"
#include "buffer_cache/alt.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/new_semaphore.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/scoped.hpp"
#include "perfmon/perfmon.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

/* This is the main entry point for performing B-tree operations. */

namespace profile {
class trace_t;
}

class buf_parent_t;
class cache_t;
class value_deleter_t;

enum cache_snapshotted_t { CACHE_SNAPSHOTTED_NO, CACHE_SNAPSHOTTED_YES };

/* An abstract superblock provides the starting point for performing B-tree operations.
This makes it so that the B-tree code doesn't actually have to know about the format of
the superblock, or about anything else that might be contained in the superblock besides
the root block ID and the stat block ID. */
class superblock_t {
public:
    superblock_t() { }
    virtual ~superblock_t() { }
    // Release the superblock if possible (otherwise do nothing)
    virtual void release() = 0;

    virtual block_id_t get_root_block_id() = 0;
    virtual void set_root_block_id(block_id_t new_root_block) = 0;

    /* If stats collection is desired, create a stat block with `create_stat_block()` and
    store its ID on the superblock, then return it from `get_stat_block_id()`. If stats
    collection is not desired, `get_stat_block_id()` can always return `NULL_BLOCK_ID`.
    */
    virtual block_id_t get_stat_block_id() = 0;

    virtual buf_parent_t expose_buf() = 0;

    cache_t *cache() { return expose_buf().cache(); }

private:
    DISABLE_COPYING(superblock_t);
};

class keyvalue_location_t {
public:
    keyvalue_location_t()
        : superblock(nullptr), pass_back_superblock(nullptr),
          there_originally_was_value(false), stat_block(NULL_BLOCK_ID) { }

    ~keyvalue_location_t() {
        if (superblock != nullptr) {
            if (pass_back_superblock != nullptr) {
                pass_back_superblock->pulse(superblock);
            } else {
                superblock->release();
            }
        }
    }

    superblock_t *superblock;

    promise_t<superblock_t *> *pass_back_superblock;

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc_t<void> value;

    template <class T>
    T *value_as() { return static_cast<T *>(value.get()); }

    // Stat block when modifications are made using this class the statblock is
    // update.
    block_id_t stat_block;
private:

    DISABLE_COPYING(keyvalue_location_t);
};

buf_lock_t get_root(value_sizer_t *sizer, superblock_t *sb);

void check_and_handle_split(value_sizer_t *sizer,
                            buf_lock_t *buf,
                            buf_lock_t *last_buf,
                            superblock_t *sb,
                            const btree_key_t *key, void *new_value,
                            const value_deleter_t *detacher);

void check_and_handle_underfull(value_sizer_t *sizer,
                                buf_lock_t *buf,
                                buf_lock_t *last_buf,
                                superblock_t *sb,
                                const btree_key_t *key,
                                const value_deleter_t *detacher);

/* Set sb to have root id as its root block and release sb */
void insert_root(block_id_t root_id, superblock_t *sb);

/* Create a stat block suitable for storing in a superblock and returning from
`get_stat_block_id()`. */
block_id_t create_stat_block(buf_parent_t parent);

/* Note that there's no guarantee that `pass_back_superblock` will have been
 * pulsed by the time `find_keyvalue_location_for_write` returns. In some cases,
 * the superblock is returned only when `*keyvalue_location_out` gets destructed. */
void find_keyvalue_location_for_write(
        value_sizer_t *sizer,
        superblock_t *superblock,
        const btree_key_t *key,
        repli_timestamp_t timestamp,
        const value_deleter_t *balancing_detacher,
        keyvalue_location_t *keyvalue_location_out,
        profile::trace_t *trace,
        promise_t<superblock_t *> *pass_back_superblock = nullptr) THROWS_NOTHING;

void find_keyvalue_location_for_read(
        value_sizer_t *sizer,
        superblock_t *superblock,
        const btree_key_t *key,
        keyvalue_location_t *keyvalue_location_out,
        btree_stats_t *stats,
        profile::trace_t *trace);

/* `delete_mode_t` controls how `apply_keyvalue_change()` acts when `kv_loc->value` is
empty. */
enum class delete_mode_t {
    /* If there was a value before, remove it and add a tombstone. (If `tstamp` is less
    than the cutpoint, no tombstone will be added.) Otherwise, do nothing. This mode is
    used for regular delete queries. */
    REGULAR_QUERY,
    /* If there was a value or tombstone before, remove it. This mode is used for erasing
    ranges of the database (e.g. during resharding) and also sometimes in backfilling. */
    ERASE,
    /* If there was a value or tombstone before, remove it. Then add a tombstone,
    regardless of what was present before, unless `tstamp` is less than the cutpoint.
    This mode is used for transferring tombstones from other servers in backfilling. */
    MAKE_TOMBSTONE
};

void apply_keyvalue_change(
        value_sizer_t *sizer,
        keyvalue_location_t *kv_loc,
        const btree_key_t *key,
        repli_timestamp_t tstamp,
        const value_deleter_t *balancing_detacher,
        delete_mode_t delete_mode);

#endif  // BTREE_OPERATIONS_HPP_
