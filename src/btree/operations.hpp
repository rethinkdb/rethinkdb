#ifndef __BTREE_OPERATIONS_HPP__
#define __BTREE_OPERATIONS_HPP__

#include "errors.hpp"
#include <boost/scoped_ptr.hpp>

#include "utils.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "containers/scoped_malloc.hpp"
#include "btree/node.hpp"
#include "btree/leaf_node.hpp"

class btree_slice_t;

/* An abstract superblock provides the starting point for performing btree operations */
class superblock_t {
public:
    superblock_t() { }
    virtual ~superblock_t() { }
    // Release the superblock if possible (otherwise do nothing)
    virtual void release() = 0;
    // If we hold a lock on a super block, swap it into swapee
    // (might swap in an empty buf_lock_t if we don't have an actual superblock)
    virtual void swap_buf(buf_lock_t &swapee) = 0;
    virtual block_id_t get_root_block_id() const = 0;
    virtual void set_root_block_id(const block_id_t new_root_block) = 0;
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
    buf_lock_t* get() { return &sb_buf_; }
    void swap_buf(buf_lock_t &swapee);
    block_id_t get_root_block_id() const;
    void set_root_block_id(const block_id_t new_root_block);
    block_id_t get_delete_queue_block() const;
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
    void swap_buf(buf_lock_t &swapee) {
        // Swap with empty buf_lock
        buf_lock_t tmp;
        tmp.swap(swapee);
    }
    block_id_t get_root_block_id() const {
        return root_block_id_;
    }
    void set_root_block_id(const block_id_t new_root_block) {
        root_block_id_ = new_root_block;
    }
    block_id_t get_delete_queue_block() const {
        return NULL_BLOCK_ID;
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

class got_superblock_t {
public:
    got_superblock_t() { }
    explicit got_superblock_t(superblock_t * sb_) : sb(sb_) { }

    boost::scoped_ptr<superblock_t> sb;

private:
    DISABLE_COPYING(got_superblock_t);
};

template <class Value>
class keyvalue_location_t {
public:
    keyvalue_location_t() : there_originally_was_value(false) { }

    boost::scoped_ptr<superblock_t> sb;

    // The parent buf of buf, if buf is not the root node.  This is hacky.
    buf_lock_t last_buf;

    // The buf owning the leaf node which contains the value.
    buf_lock_t buf;

    bool there_originally_was_value;
    // If the key/value pair was found, a pointer to a copy of the
    // value, otherwise NULL.
    scoped_malloc<Value> value;

    void swap(keyvalue_location_t& other) {
        sb.swap(other.sb);
        last_buf.swap(other.last_buf);
        buf.swap(other.buf);
        std::swap(there_originally_was_value, other.there_originally_was_value);
        value.swap(other.value);
    }

private:
    DISABLE_COPYING(keyvalue_location_t);
};



template <class Value>
class key_modification_callback_t {
public:
    // Perhaps this modifies the kv_loc in place, swapping in its own
    // scoped_malloc.  It's the caller's responsibility to have
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
class value_txn_t {
public:
    value_txn_t(btree_key_t *, keyvalue_location_t<Value>&, repli_timestamp_t, key_modification_callback_t<Value> *km_callback, eviction_priority_t *root_eviction_priority);
    value_txn_t(btree_slice_t *slice, sequence_group_t *seq_group, btree_key_t *key, const repli_timestamp_t tstamp, const order_token_t token, key_modification_callback_t<Value> *km_callback);

    ~value_txn_t();

    scoped_malloc<Value>& value();

    transaction_t *get_txn();

private:
    btree_key_t *key;
    boost::scoped_ptr<transaction_t> txn;
    keyvalue_location_t<Value> kv_location;
    repli_timestamp_t tstamp;
    eviction_priority_t *root_eviction_priority;

    // Not owned by this object.
    key_modification_callback_t<Value> *km_callback;

    DISABLE_COPYING(value_txn_t);
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
    typedef std::pair<sz_t,char*> key_t;
    typedef std::pair<sz_t,char*> value_t;

    superblock_metainfo_iterator_t(char* metainfo, char* metainfo_end) : end(metainfo_end) { advance(metainfo); }
    explicit superblock_metainfo_iterator_t(std::vector<char>& metainfo) : end(metainfo.data() + metainfo.size()) { advance(metainfo.data()); }

    bool is_end() { return pos == end; }

    void operator++();
    void operator++(int) { this->operator++(); }

    std::pair<key_t,value_t> operator*() {
        return std::make_pair(key(), value());
    }
    key_t key() { return std::make_pair(key_size, key_ptr); }
    value_t value() { return std::make_pair(value_size, value_ptr); }

    char* record_ptr() { return pos; }
    char* next_record_ptr() { return next_pos; }
    char* end_ptr() { return end; }
    sz_t* value_size_ptr() { return reinterpret_cast<sz_t*>(value_ptr) - 1; }
private:
    void advance(char* p);

    char* pos;
    char* next_pos;
    char* end;
    sz_t key_size;
    char* key_ptr;
    sz_t value_size;
    char* value_ptr;
};

bool get_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key, std::vector<char> &value_out);
void get_superblock_metainfo(transaction_t *txn, buf_t *superblock, std::vector<std::pair<std::vector<char>,std::vector<char> > > &kv_pairs_out);

void set_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key, const std::vector<char> &value);

void delete_superblock_metainfo(transaction_t *txn, buf_t *superblock, const std::vector<char> &key);
void clear_superblock_metainfo(transaction_t *txn, buf_t *superblock);

#include "btree/operations.tcc"

#endif  // __BTREE_OPERATIONS_HPP__
