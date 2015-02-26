// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_OPERATIONS_HPP_
#define BTREE_OPERATIONS_HPP_

#include <algorithm>
#include <utility>
#include <vector>

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
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

    virtual block_id_t get_stat_block_id() = 0;
    virtual void set_stat_block_id(block_id_t new_stat_block) = 0;

    virtual buf_parent_t expose_buf() = 0;

    cache_t *cache() { return expose_buf().cache(); }

private:
    DISABLE_COPYING(superblock_t);
};

class btree_stats_t {
public:
    explicit btree_stats_t(perfmon_collection_t *parent,
                           const std::string &identifier)
        : btree_collection(),
          pm_keys_read(secs_to_ticks(1)),
          pm_keys_set(secs_to_ticks(1)),
          pm_keys_membership(&btree_collection,
              &pm_keys_read, "keys_read",
              &pm_total_keys_read, "total_keys_read",
              &pm_keys_set, "keys_set",
              &pm_total_keys_set, "total_keys_set") {
        if (parent != NULL) {
            rename(parent, identifier);
        }
    }

    void hide() {
        btree_collection_membership.reset();
    }

    void rename(perfmon_collection_t *parent,
                const std::string &identifier) {
        btree_collection_membership.reset();
        btree_collection_membership.init(new perfmon_membership_t(
            parent,
            &btree_collection,
            "btree-" + identifier));
    }

    perfmon_collection_t btree_collection;
    scoped_ptr_t<perfmon_membership_t> btree_collection_membership;
    perfmon_rate_monitor_t
        pm_keys_read,
        pm_keys_set;
    perfmon_counter_t
        pm_total_keys_read,
        pm_total_keys_set;
    perfmon_multi_membership_t pm_keys_membership;
};

class keyvalue_location_t {
public:
    keyvalue_location_t()
        : superblock(NULL), pass_back_superblock(NULL),
          there_originally_was_value(false), stat_block(NULL_BLOCK_ID),
          stats(NULL) { }

    ~keyvalue_location_t() {
        if (pass_back_superblock != NULL && superblock != NULL) {
            pass_back_superblock->pulse(superblock);
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

    btree_stats_t *stats;
private:

    DISABLE_COPYING(keyvalue_location_t);
};


// KSI: This type is stupid because the only subclass is
// null_key_modification_callback_t?
class key_modification_callback_t {
public:
    // Perhaps this modifies the kv_loc in place, swapping in its own
    // scoped_malloc_t.  It's the caller's responsibility to have
    // destroyed any blobs that the value might reference, before
    // calling this here, so that this callback can reacquire them.
    virtual key_modification_proof_t value_modification(keyvalue_location_t *kv_loc, const btree_key_t *key) = 0;

    key_modification_callback_t() { }
protected:
    virtual ~key_modification_callback_t() { }
private:
    DISABLE_COPYING(key_modification_callback_t);
};




class null_key_modification_callback_t : public key_modification_callback_t {
    key_modification_proof_t
    value_modification(UNUSED keyvalue_location_t *kv_loc,
                       UNUSED const btree_key_t *key) {
        // do nothing
        return key_modification_proof_t::real_proof();
    }
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

/* Create a stat block for the superblock. */
void create_stat_block(superblock_t *sb);

/* Note that there's no guarantee that `pass_back_superblock` will have been
 * pulsed by the time `find_keyvalue_location_for_write` returns. In some cases,
 * the superblock is returned only when `*keyvalue_location_out` gets destructed. */
void find_keyvalue_location_for_write(
        value_sizer_t *sizer,
        superblock_t *superblock, const btree_key_t *key,
        const value_deleter_t *balancing_detacher,
        keyvalue_location_t *keyvalue_location_out,
        btree_stats_t *stats,
        profile::trace_t *trace,
        promise_t<superblock_t *> *pass_back_superblock = NULL) THROWS_NOTHING;

void find_keyvalue_location_for_read(
        value_sizer_t *sizer,
        superblock_t *superblock, const btree_key_t *key,
        keyvalue_location_t *keyvalue_location_out,
        btree_stats_t *stats, profile::trace_t *trace);

/* Specifies whether `apply_keyvalue_change` should delete or erase a value.
The difference is that deleting a value updates the node's replication timestamp
and creates a deletion entry in the leaf. This means that the deletion is going
to be backfilled.
An erase on the other hand just removes the keyvalue from the leaf node, as if
it had never existed.
If the change is the result of a ReQL query, you'll almost certainly want DELETE
and not ERASE. */
enum class delete_or_erase_t { DELETE, ERASE };

void apply_keyvalue_change(
        value_sizer_t *sizer,
        keyvalue_location_t *kv_loc,
        const btree_key_t *key, repli_timestamp_t tstamp,
        const value_deleter_t *balancing_detacher,
        key_modification_callback_t *km_callback,
        delete_or_erase_t delete_or_erase = delete_or_erase_t::DELETE);

#endif  // BTREE_OPERATIONS_HPP_
