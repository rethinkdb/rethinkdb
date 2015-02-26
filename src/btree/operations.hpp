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
#include "repli_timestamp.hpp"
#include "utils.hpp"

namespace profile {
class trace_t;
}

class btree_slice_t;
class binary_blob_t;
class value_deleter_t;

template <class> class promise_t;

enum cache_snapshotted_t { CACHE_SNAPSHOTTED_NO, CACHE_SNAPSHOTTED_YES };

/* An abstract superblock provides the starting point for performing btree operations */
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

    virtual block_id_t get_sindex_block_id() = 0;
    virtual void set_sindex_block_id(block_id_t new_block_id) = 0;

    virtual buf_parent_t expose_buf() = 0;

    cache_t *cache() { return expose_buf().cache(); }

private:
    DISABLE_COPYING(superblock_t);
};

/* real_superblock_t implements superblock_t in terms of an actual on-disk block
   structure. */
class real_superblock_t : public superblock_t {
public:
    explicit real_superblock_t(buf_lock_t &&sb_buf);
    real_superblock_t(buf_lock_t &&sb_buf, new_semaphore_acq_t &&write_semaphore_acq);

    void release();
    buf_lock_t *get() { return &sb_buf_; }

    block_id_t get_root_block_id();
    void set_root_block_id(block_id_t new_root_block);

    block_id_t get_stat_block_id();
    void set_stat_block_id(block_id_t new_stat_block);

    block_id_t get_sindex_block_id();
    void set_sindex_block_id(block_id_t new_block_id);

    buf_parent_t expose_buf() { return buf_parent_t(&sb_buf_); }

private:
    /* The write_semaphore_acq_ is empty for reads.
    For writes it locks the write superblock acquisition semaphore until the
    sb_buf_ is released.
    Note that this is used to throttle writes compared to reads, but not required
    for correctness. */    
    new_semaphore_acq_t write_semaphore_acq_;

    buf_lock_t sb_buf_;
};

class btree_stats_t;

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


/* This iterator encapsulates most of the metainfo data layout. Unfortunately,
 * functions set_superblock_metainfo and delete_superblock_metainfo also know a
 * lot about the data layout, so if it's changed, these functions must be
 * changed as well.
 *
 * Data layout is dead simple right now, it's an array of the following
 * (unaligned, unpadded) contents:
 *
 *   sz_t key_size;
 *   char key[key_size];
 *   sz_t value_size;
 *   char value[value_size];
 */
struct superblock_metainfo_iterator_t {
    typedef uint32_t sz_t;  // be careful: the values of this type get casted to int64_t in checks, so it must fit
    typedef std::pair<sz_t, char *> key_t;
    typedef std::pair<sz_t, char *> value_t;

    superblock_metainfo_iterator_t(char *metainfo, char *metainfo_end) : end(metainfo_end) { advance(metainfo); }

    bool is_end() { return pos == end; }

    void operator++();

    std::pair<key_t, value_t> operator*() {
        return std::make_pair(key(), value());
    }
    key_t key() { return std::make_pair(key_size, key_ptr); }
    value_t value() { return std::make_pair(value_size, value_ptr); }

    char *record_ptr() { return pos; }
    char *next_record_ptr() { return next_pos; }
    char *end_ptr() { return end; }
    sz_t *value_size_ptr() { return reinterpret_cast<sz_t*>(value_ptr) - 1; }
private:
    void advance(char *p);

    char *pos;
    char *next_pos;
    char *end;
    sz_t key_size;
    char *key_ptr;
    sz_t value_size;
    char *value_ptr;
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

// Metainfo functions
bool get_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             std::vector<char> *value_out);

void get_superblock_metainfo(
    buf_lock_t *superblock,
    std::vector< std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out);

void set_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<char> &key,
                             const binary_blob_t &value);

void set_superblock_metainfo(buf_lock_t *superblock,
                             const std::vector<std::vector<char> > &keys,
                             const std::vector<binary_blob_t> &values);

void delete_superblock_metainfo(buf_lock_t *superblock,
                                const std::vector<char> &key);
void clear_superblock_metainfo(buf_lock_t *superblock);

/* Set sb to have root id as its root block and release sb */
void insert_root(block_id_t root_id, superblock_t *sb);

/* Create a stat block for the superblock. */
void create_stat_block(superblock_t *sb);

void get_btree_superblock(
        txn_t *txn,
        access_t access,
        scoped_ptr_t<real_superblock_t> *got_superblock_out);
/* Variant for writes that go through a superblock write semaphore */
void get_btree_superblock(
        txn_t *txn,
        write_access_t access,
        new_semaphore_acq_t &&write_sem_acq,
        scoped_ptr_t<real_superblock_t> *got_superblock_out);

void get_btree_superblock_and_txn_for_writing(
        cache_conn_t *cache_conn,
        new_semaphore_t *superblock_write_semaphore,
        write_access_t superblock_access,
        int expected_change_count,
        repli_timestamp_t tstamp,
        write_durability_t durability,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out);

void get_btree_superblock_and_txn_for_backfilling(
        cache_conn_t *cache_conn,
        cache_account_t *backfill_account,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out);

void get_btree_superblock_and_txn_for_reading(
        cache_conn_t *cache_conn,
        cache_snapshotted_t snapshotted,
        scoped_ptr_t<real_superblock_t> *got_superblock_out,
        scoped_ptr_t<txn_t> *txn_out);

/* Note that there's no guarantee that `pass_back_superblock` will have been
 * pulsed by the time `find_keyvalue_location_for_write` returns. In some cases,
 * the superblock is returned only when `*kevaluey_location_out` gets destructed. */
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
