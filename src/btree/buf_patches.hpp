// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BTREE_BUF_PATCHES_HPP_
#define BTREE_BUF_PATCHES_HPP_

/* This file provides btree specific buffer patches */

#include "btree/keys.hpp"
#include "buffer_cache/buf_patch.hpp"
#include "containers/scoped.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

template <class V> class value_sizer_t;

class key_modification_proof_t;


/* Erase a key/value pair from a leaf node, when this is an idempotent
   operation.  Does not leave deletion history behind. */
class leaf_erase_presence_patch_t : public buf_patch_t {
public:
    leaf_erase_presence_patch_t(block_id_t block_id, const char *data, uint16_t data_length);

    virtual void apply_to_buf(char *buf_data, block_size_t bs);

protected:
    virtual void serialize_data(char *destination) const;
    virtual uint16_t get_data_size() const;

private:
    friend void leaf_patched_erase_presence(buf_lock_t *node, const btree_key_t *key, key_modification_proof_t km_proof);
    leaf_erase_presence_patch_t(block_id_t block_id, const btree_key_t *_key);

    store_key_t key;
};

#endif /* BTREE_BUF_PATCHES_HPP_ */

