#ifndef BTREE_OPERATIONS_HPP_
#define BTREE_OPERATIONS_HPP_

#include <algorithm>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "utils.hpp"
#include "containers/scoped.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "repli_timestamp.hpp"

class btree_slice_t;

enum cache_snapshotted_t { CACHE_SNAPSHOTTED_NO, CACHE_SNAPSHOTTED_YES };

/* An abstract superblock provides the starting point for performing btree operations */
class superblock_t {
public:
    superblock_t() { }
    virtual ~superblock_t() { }
    // Release the superblock if possible (otherwise do nothing)
    virtual void release() = 0;

    virtual block_id_t get_root_block_id() const = 0;
    virtual void set_root_block_id(const block_id_t new_root_block) = 0;

    virtual block_id_t get_stat_block_id() const = 0;
    virtual void set_stat_block_id(block_id_t new_stat_block) = 0;

    virtual void set_eviction_priority(eviction_priority_t eviction_priority) = 0;
    virtual eviction_priority_t get_eviction_priority() = 0;

private:
    DISABLE_COPYING(superblock_t);
};

/* real_superblock_t implements superblock_t in terms of an actual on-disk block
   structure. */
class real_superblock_t : public superblock_t {
public:
    explicit real_superblock_t(buf_lock_t &sb_buf);

    void release();
    buf_lock_t *get() { return &sb_buf_; }

    block_id_t get_root_block_id() const;
    void set_root_block_id(const block_id_t new_root_block);

    block_id_t get_stat_block_id() const;
    void set_stat_block_id(block_id_t new_stat_block);

    void set_eviction_priority(eviction_priority_t eviction_priority);
    eviction_priority_t get_eviction_priority();

private:
    buf_lock_t sb_buf_;
};

/* This is for nested btrees, where the "superblock" is really more like a super value.
 It provides an in-memory superblock replacement.

 Note for use for nested btrees: If you want to nest a tree into some super value,
 you would probably have a block_id_t nested_root value in the super value. Then,
 before accessing the nested tree, you can construct a virtual_superblock_t
 based on the nested_root value. Once write operations to the nested btree have
 finished, you should check whether the root_block_id has been changed,
 and if it has, use get_root_block_id() to update the nested_root value in the
 super block.
 */
class virtual_superblock_t : public superblock_t {
public:
    explicit virtual_superblock_t(block_id_t root_block_id = NULL_BLOCK_ID) : root_block_id_(root_block_id) { }

    void release() { }
    block_id_t get_root_block_id() const {
        return root_block_id_;
    }
    void set_root_block_id(const block_id_t new_root_block) {
        root_block_id_ = new_root_block;
    }

    block_id_t get_stat_block_id() const {
        crash("Not implemented\n");
    }

    void set_stat_block_id(block_id_t) {
        crash("Not implemented\n");
    }

    void set_eviction_priority(UNUSED eviction_priority_t eviction_priority) {
        // TODO Actually support the setting and getting of eviction priority in a virtual superblock.
    }

    eviction_priority_t get_eviction_priority() {
        // TODO Again, actually support the setting and getting of eviction priority in a virtual superblock.
        return FAKE_EVICTION_PRIORITY;
    }

private:
    block_id_t root_block_id_;
};

class btree_stats_t;

template <class Value>
class keyvalue_location_t {
public:
    keyvalue_location_t() : there_originally_was_value(false), stat_block(NULL_BLOCK_ID), stats(NULL) { }

    superblock_t *superblock;

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc_t<Value> value;

    void swap(keyvalue_location_t& other) {
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



template <class Value>
class key_modification_callback_t {
public:
    // Perhaps this modifies the kv_loc in place, swapping in its own
    // scoped_malloc_t.  It's the caller's responsibility to have
    // destroyed any blobs that the value might reference, before
    // calling this here, so that this callback can reacquire them.
    virtual key_modification_proof_t value_modification(transaction_t *txn, keyvalue_location_t<Value> *kv_loc, const btree_key_t *key) = 0;

    key_modification_callback_t() { }
protected:
    virtual ~key_modification_callback_t() { }
private:
    DISABLE_COPYING(key_modification_callback_t);
};




template <class Value>
class null_key_modification_callback_t : public key_modification_callback_t<Value> {
    key_modification_proof_t value_modification(UNUSED transaction_t *txn, UNUSED keyvalue_location_t<Value> *kv_loc, UNUSED const btree_key_t *key) {
        // do nothing
        return key_modification_proof_t::real_proof();
    }
};

// TODO: Remove all instances of this, each time considering what kind
// of key modification callback is necessary.
template <class Value>
class fake_key_modification_callback_t : public key_modification_callback_t<Value> {
    key_modification_proof_t value_modification(UNUSED transaction_t *txn, UNUSED keyvalue_location_t<Value> *kv_loc, UNUSED const btree_key_t *key) {
        // do nothing
        return key_modification_proof_t::fake_proof();
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
    explicit superblock_metainfo_iterator_t(std::vector<char>& metainfo) : end(metainfo.data() + metainfo.size()) { advance(metainfo.data()); }

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

void get_root(value_sizer_t<void> *sizer, transaction_t *txn, superblock_t* sb, buf_lock_t *buf_out, eviction_priority_t root_eviction_priority);

void check_and_handle_split(value_sizer_t<void> *sizer, transaction_t *txn, buf_lock_t& buf, buf_lock_t& last_buf, superblock_t *sb,
                            const btree_key_t *key, void *new_value, eviction_priority_t *root_eviction_priority);

void check_and_handle_underfull(value_sizer_t<void> *sizer, transaction_t *txn,
                                buf_lock_t& buf, buf_lock_t& last_buf, superblock_t *sb,
                                const btree_key_t *key);

bool get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, std::vector<char> &value_out);
void get_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, std::vector< std::pair<std::vector<char>, std::vector<char> > > &kv_pairs_out);

void set_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key, const std::vector<char> &value);

void delete_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock, const std::vector<char> &key);
void clear_superblock_metainfo(transaction_t *txn, buf_lock_t *superblock);

/* Set sb to have root id as its root block and release sb */
void insert_root(block_id_t root_id, superblock_t* sb);

/* Create a stat block for the superblock if it doesn't already have one. */
void ensure_stat_block(transaction_t *txn, superblock_t *sb, eviction_priority_t stat_block_eviction_priority);

void get_btree_superblock(transaction_t *txn, access_t access, boost::scoped_ptr<real_superblock_t> *got_superblock_out);

void get_btree_superblock(btree_slice_t *slice, access_t access, int expected_change_count,
                          repli_timestamp_t tstamp, order_token_t token, cache_snapshotted_t snapshotted,
                          const boost::shared_ptr<cache_account_t> &cache_account,
                          boost::scoped_ptr<real_superblock_t> *got_superblock_out, boost::scoped_ptr<transaction_t>& txn_out);

void get_btree_superblock(btree_slice_t *slice, access_t access, int expected_change_count, 
                                 repli_timestamp_t tstamp, order_token_t token,
                                 boost::scoped_ptr<real_superblock_t> *got_superblock_out, 
                                 boost::scoped_ptr<transaction_t>& txn_out);

void get_btree_superblock_for_backfilling(btree_slice_t *slice, order_token_t token, 
                                                 boost::scoped_ptr<real_superblock_t> *got_superblock_out, 
                                                 boost::scoped_ptr<transaction_t>& txn_out);

void get_btree_superblock_for_reading(btree_slice_t *slice, access_t access, order_token_t token,
                                      cache_snapshotted_t snapshotted, boost::scoped_ptr<real_superblock_t> *got_superblock_out,
                                      boost::scoped_ptr<transaction_t>& txn_out);

#include "btree/operations.tcc"

#endif  // BTREE_OPERATIONS_HPP_
