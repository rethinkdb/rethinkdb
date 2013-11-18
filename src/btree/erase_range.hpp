// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BTREE_ERASE_RANGE_HPP_
#define BTREE_ERASE_RANGE_HPP_

#include "errors.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"  // RSI: for SLICE_ALT
#include "buffer_cache/types.hpp"

class btree_slice_t;
struct store_key_t;
struct btree_key_t;
class order_token_t;
class superblock_t;
class signal_t;

class key_tester_t {
public:
    key_tester_t() { }
    virtual bool key_should_be_erased(const btree_key_t *key) = 0;

protected:
    virtual ~key_tester_t() { }
private:
    DISABLE_COPYING(key_tester_t);
};

struct always_true_key_tester_t : public key_tester_t {
    bool key_should_be_erased(UNUSED const btree_key_t *key) {
        return true;
    }
};

class value_deleter_t {
public:
    value_deleter_t() { }
#if SLICE_ALT
    virtual void delete_value(alt::alt_buf_parent_t leaf_node, void *value) = 0;
#else
    virtual void delete_value(transaction_t *txn, void *value) = 0;
#endif

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

#if SLICE_ALT
void btree_erase_range_generic(value_sizer_t<void> *sizer, btree_slice_t *slice,
                               key_tester_t *tester,
                               value_deleter_t *deleter,
                               const btree_key_t *left_exclusive_or_null,
                               const btree_key_t *right_inclusive_or_null,
                               superblock_t *superblock,
                               signal_t *interruptor,
                               bool release_superblock = true);
#else
void btree_erase_range_generic(value_sizer_t<void> *sizer, btree_slice_t *slice,
                               key_tester_t *tester,
                               value_deleter_t *deleter,
                               const btree_key_t *left_exclusive_or_null,
                               const btree_key_t *right_inclusive_or_null,
                               transaction_t *txn, superblock_t *superblock,
                               signal_t *interruptor,
                               bool release_superblock = true);
#endif

#if SLICE_ALT
void erase_all(value_sizer_t<void> *sizer, btree_slice_t *slice,
               value_deleter_t *deleter,
               superblock_t *superblock,
               signal_t *interruptor,
               bool release_superblock = true);
#else
void erase_all(value_sizer_t<void> *sizer, btree_slice_t *slice,
               value_deleter_t *deleter, transaction_t *txn,
               superblock_t *superblock,
               signal_t *interruptor,
               bool release_superblock = true);
#endif

#endif  // BTREE_ERASE_RANGE_HPP_
