#ifndef BTREE_BUF_PATCHES_HPP_
#define BTREE_BUF_PATCHES_HPP_

/* This file provides btree specific buffer patches */

#include "btree/keys.hpp"
#include "buffer_cache/buf_patch.hpp"
#include "containers/scoped_malloc.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

template <class V> class value_sizer_t;

class key_modification_proof_t;


/* Insert and/or replace a key/value pair in a leaf node */
class leaf_insert_patch_t : public buf_patch_t {
public:
    leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length);

    virtual void apply_to_buf(char* buf_data, block_size_t bs);

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    friend void leaf_patched_insert(value_sizer_t<void> *sizer, buf_lock_t *node, const btree_key_t *key, const void *value, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

    leaf_insert_patch_t(block_id_t block_id, repli_timestamp_t block_timestamp, patch_counter_t patch_counter, uint16_t value_size, const void *value, const btree_key_t *key, repli_timestamp_t insertion_time);

    uint16_t value_size;
    scoped_malloc_t<char> value_buf;
    store_key_t key;
    repli_timestamp_t insertion_time;
};

/* Remove a key/value pair from a leaf node */
class leaf_remove_patch_t : public buf_patch_t {
public:
    leaf_remove_patch_t(const block_id_t block_id, patch_counter_t patch_counter, const char* data, uint16_t data_length);

    virtual void apply_to_buf(char* buf_data, block_size_t bs);

protected:
    virtual void serialize_data(char* destination) const;
    virtual uint16_t get_data_size() const;

private:
    friend void leaf_patched_remove(buf_lock_t *node, const btree_key_t *key, repli_timestamp_t tstamp, key_modification_proof_t km_proof);
    leaf_remove_patch_t(block_id_t block_id, repli_timestamp_t block_timestamp, patch_counter_t patch_counter, repli_timestamp_t tstamp, const btree_key_t *key);

    repli_timestamp_t timestamp;
    store_key_t key;
};

/* Erase a key/value pair from a leaf node, when this is an idempotent
   operation.  Does not leave deletion history behind. */
class leaf_erase_presence_patch_t : public buf_patch_t {
public:
    leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, const char *data, uint16_t data_length);

    virtual void apply_to_buf(char *buf_data, block_size_t bs);

protected:
    virtual void serialize_data(char *destination) const;
    virtual uint16_t get_data_size() const;

private:
    friend void leaf_patched_erase_presence(buf_lock_t *node, const btree_key_t *key, key_modification_proof_t km_proof);
    leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, const btree_key_t *_key);

    store_key_t key;
};

#endif /* BTREE_BUF_PATCHES_HPP_ */

