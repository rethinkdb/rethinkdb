// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_OPERATIONS_HPP_
#define BTREE_OPERATIONS_HPP_

#include <algorithm>
#include <utility>
#include <vector>

#include "btree/leaf_node.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"  // RSI: Remove.  for SLICE_ALT
#if SLICE_ALT
#include "buffer_cache/alt/alt.hpp"
#else
#include "buffer_cache/buffer_cache.hpp"
#endif
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/promise.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/scoped.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

// RSI: This code doesn't use the notion of "parent transaction" at all, and it must.

class btree_slice_t;

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

#if !SLICE_ALT
    virtual void set_eviction_priority(eviction_priority_t eviction_priority) = 0;
    virtual eviction_priority_t get_eviction_priority() = 0;
#endif

#if SLICE_ALT
    // RSI: Add buf_lock_parent_t or something.
    virtual alt::alt_buf_parent_t expose_buf() = 0;
#endif

private:
    DISABLE_COPYING(superblock_t);
};

/* real_superblock_t implements superblock_t in terms of an actual on-disk block
   structure. */
class real_superblock_t : public superblock_t {
public:
#if SLICE_ALT
    explicit real_superblock_t(alt::alt_buf_lock_t &&sb_buf);
#else
    explicit real_superblock_t(buf_lock_t *sb_buf);
#endif

    void release();
#if SLICE_ALT
    alt::alt_buf_lock_t *get() { return &sb_buf_; }
#else
    buf_lock_t *get() { return &sb_buf_; }
#endif

    block_id_t get_root_block_id();
    void set_root_block_id(block_id_t new_root_block);

    block_id_t get_stat_block_id();
    void set_stat_block_id(block_id_t new_stat_block);

    block_id_t get_sindex_block_id();
    void set_sindex_block_id(block_id_t new_block_id);

#if !SLICE_ALT
    void set_eviction_priority(eviction_priority_t eviction_priority);
    eviction_priority_t get_eviction_priority();
#endif

#if SLICE_ALT
    alt::alt_buf_parent_t expose_buf() { return alt::alt_buf_parent_t(&sb_buf_); }
#endif

private:
#if SLICE_ALT
    alt::alt_buf_lock_t sb_buf_;
#else
    buf_lock_t sb_buf_;
#endif
};

class btree_stats_t;

template <class Value>
class key_modification_callback_t;

template <class Value>
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
#if SLICE_ALT
    alt::alt_buf_lock_t last_buf;
#else
    buf_lock_t last_buf;
#endif

    // The buf owning the leaf node which contains the value.
#if SLICE_ALT
    alt::alt_buf_lock_t buf;
#else
    buf_lock_t buf;
#endif

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc_t<Value> value;

    void swap(keyvalue_location_t &other) {
        std::swap(superblock, other.superblock);
        std::swap(stat_block, other.stat_block);
        last_buf.swap(other.last_buf);
        buf.swap(other.buf);
        std::swap(there_originally_was_value, other.there_originally_was_value);
        std::swap(stats, other.stats);
        value.swap(other.value);
    }


    //Stat block when modifications are made using this class the statblock is update
    block_id_t stat_block;

    btree_stats_t *stats;
private:

    DISABLE_COPYING(keyvalue_location_t);
};


// RSI: This type is stupid because the only subclass is
// null_key_modification_callback_t is null_key_modification_callback_t?
template <class Value>
class key_modification_callback_t {
public:
    // Perhaps this modifies the kv_loc in place, swapping in its own
    // scoped_malloc_t.  It's the caller's responsibility to have
    // destroyed any blobs that the value might reference, before
    // calling this here, so that this callback can reacquire them.
#if SLICE_ALT
    virtual key_modification_proof_t value_modification(keyvalue_location_t<Value> *kv_loc, const btree_key_t *key) = 0;
#else
    virtual key_modification_proof_t value_modification(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, const btree_key_t *key) = 0;
#endif

    key_modification_callback_t() { }
protected:
    virtual ~key_modification_callback_t() { }
private:
    DISABLE_COPYING(key_modification_callback_t);
};




template <class Value>
class null_key_modification_callback_t : public key_modification_callback_t<Value> {
#if SLICE_ALT
    key_modification_proof_t
    value_modification(UNUSED keyvalue_location_t<Value> *kv_loc,
                       UNUSED const btree_key_t *key) {
#else
    key_modification_proof_t value_modification(UNUSED transaction_t *txn, UNUSED keyvalue_location_t<Value> *kv_loc, UNUSED const btree_key_t *key) {
#endif
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

#if SLICE_ALT
// RSI: Have this return the buf_lock_t.
void get_root(value_sizer_t<void> *sizer, superblock_t *sb,
              alt::alt_buf_lock_t *buf_out);
#endif
void get_root(value_sizer_t<void> *sizer, transaction_t *txn, superblock_t *sb, buf_lock_t *buf_out, eviction_priority_t root_eviction_priority);

#if SLICE_ALT
void check_and_handle_split(value_sizer_t<void> *sizer,
                            alt::alt_buf_lock_t *buf,
                            alt::alt_buf_lock_t *last_buf,
                            superblock_t *sb,
                            const btree_key_t *key, void *new_value);
#else
void check_and_handle_split(value_sizer_t<void> *sizer, transaction_t *txn, buf_lock_t *buf, buf_lock_t *last_buf, superblock_t *sb,
                            const btree_key_t *key, void *new_value, eviction_priority_t *root_eviction_priority);
#endif

#if SLICE_ALT
void check_and_handle_underfull(value_sizer_t<void> *sizer,
                                alt::alt_buf_lock_t *buf,
                                alt::alt_buf_lock_t *last_buf,
                                superblock_t *sb,
                                const btree_key_t *key);
#else
void check_and_handle_underfull(value_sizer_t<void> *sizer, transaction_t *txn,
                                buf_lock_t *buf, buf_lock_t *last_buf, superblock_t *sb,
                                const btree_key_t *key);
#endif

// Metainfo functions
#if SLICE_ALT
bool get_superblock_metainfo(alt::alt_buf_lock_t *superblock,
                             const std::vector<char> &key,
                             std::vector<char> *value_out);
#else
bool get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, std::vector<char> *value_out);
#endif

#if SLICE_ALT
void get_superblock_metainfo(
    alt::alt_buf_lock_t *superblock,
    std::vector< std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out);
#else
void get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, std::vector< std::pair<std::vector<char>, std::vector<char> > > *kv_pairs_out);
#endif

#if SLICE_ALT
void set_superblock_metainfo(alt::alt_buf_lock_t *superblock,
                             const std::vector<char> &key,
                             const std::vector<char> &value);
#else
void set_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, const std::vector<char> &value);
#endif

#if SLICE_ALT
void delete_superblock_metainfo(alt::alt_buf_lock_t *superblock,
                                const std::vector<char> &key);
#else
void delete_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key);
#endif
#if SLICE_ALT
void clear_superblock_metainfo(alt::alt_buf_lock_t *superblock);
#else
void clear_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock);
#endif

/* Set sb to have root id as its root block and release sb */
void insert_root(block_id_t root_id, superblock_t *sb);

/* Create a stat block for the superblock if it doesn't already have one. */
#if SLICE_ALT
void ensure_stat_block(superblock_t *sb);
#else
void ensure_stat_block(transaction_t *txn, superblock_t *sb, eviction_priority_t stat_block_eviction_priority);
#endif

#if SLICE_ALT
// RSI: return the scoped_ptr_t.
void get_btree_superblock(alt::alt_txn_t *txn, alt::alt_access_t access,
                          scoped_ptr_t<real_superblock_t> *got_superblock_out);
#else
void get_btree_superblock(transaction_t *txn, access_t access, scoped_ptr_t<real_superblock_t> *got_superblock_out);
#endif

#if SLICE_ALT
void get_btree_superblock_and_txn(btree_slice_t *slice,
                                  alt::alt_access_t superblock_access,
                                  int expected_change_count,
                                  repli_timestamp_t tstamp,
                                  order_token_t token,
                                  write_durability_t durability,
                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                  scoped_ptr_t<alt::alt_txn_t> *txn_out);
#else
void get_btree_superblock_and_txn(btree_slice_t *slice, access_t txn_access,
                                  access_t superblock_access, int expected_change_count,
                                  repli_timestamp_t tstamp, order_token_t token,
                                  write_durability_t durability,
                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                  scoped_ptr_t<transaction_t> *txn_out);
#endif

#if SLICE_ALT
void get_btree_superblock_and_txn_for_backfilling(btree_slice_t *slice, order_token_t token,
                                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                                  scoped_ptr_t<alt::alt_txn_t> *txn_out);
#else
void get_btree_superblock_and_txn_for_backfilling(btree_slice_t *slice, order_token_t token,
                                                  scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                                  scoped_ptr_t<transaction_t> *txn_out);
#endif

#if SLICE_ALT
void get_btree_superblock_and_txn_for_reading(btree_slice_t *slice,
                                              order_token_t token,
                                              cache_snapshotted_t snapshotted,
                                              scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                              scoped_ptr_t<alt::alt_txn_t> *txn_out);
#else
void get_btree_superblock_and_txn_for_reading(btree_slice_t *slice, access_t access, order_token_t token,
                                              cache_snapshotted_t snapshotted,
                                              scoped_ptr_t<real_superblock_t> *got_superblock_out,
                                              scoped_ptr_t<transaction_t> *txn_out);
#endif

#include "btree/operations.tcc"

#endif  // BTREE_OPERATIONS_HPP_
