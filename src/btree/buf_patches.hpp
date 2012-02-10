#ifndef __BTREE_BUF_PATCHES_HPP__
#define __BTREE_BUF_PATCHES_HPP__

/* This file provides btree specific buffer patches */

#include "buffer_cache/buf_patch.hpp"
#include "memcached/store.hpp"
#include "containers/scoped_malloc.hpp"

/* Btree leaf node logical patches */
struct btree_key_t;

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
    template <class V>
    friend void leaf_patched_insert(value_sizer_t<V> *sizer, buf_t *node, const btree_key_t *key, const void *value, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

    leaf_insert_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint16_t value_size, const void *value, uint8_t key_size, const char *key_contents, repli_timestamp_t insertion_time);

    uint16_t value_size;
    scoped_malloc<char> value_buf;
    scoped_malloc<btree_key_t> key_buf;
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
    friend void leaf_patched_remove(buf_t *node, const btree_key_t *key, repli_timestamp_t tstamp, key_modification_proof_t km_proof);
    leaf_remove_patch_t(block_id_t block_id, patch_counter_t patch_counter, repli_timestamp_t tstamp, uint8_t key_size, const char *key_contents);

    repli_timestamp_t timestamp;
    scoped_malloc<btree_key_t> key_buf;
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
    friend void leaf_patched_erase_presence(buf_t *node, const btree_key_t *key, key_modification_proof_t km_proof);
    leaf_erase_presence_patch_t(block_id_t block_id, patch_counter_t patch_counter, uint8_t key_size, const char *key_contents);

    scoped_malloc<btree_key_t> key_buf;
};

#endif /* __BTREE_BUF_PATCHES_HPP__ */

