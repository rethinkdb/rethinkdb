// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_ERASE_RANGE_HPP_
#define BTREE_ERASE_RANGE_HPP_

#include <functional>

#include "errors.hpp"
#include "btree/node.hpp"
#include "buffer_cache/types.hpp"

class buf_parent_t;
struct store_key_t;
struct btree_key_t;
class order_token_t;
class superblock_t;
class signal_t;
class value_deleter_t;

class value_deleter_t {
public:
    value_deleter_t() { }
    virtual void delete_value(buf_parent_t leaf_node, const void *value) const = 0;

protected:
    virtual ~value_deleter_t() { }

    DISABLE_COPYING(value_deleter_t);
};

/* A deleter that does absolutely nothing. */
class noop_value_deleter_t : public value_deleter_t {
public:
    noop_value_deleter_t() { }
    void delete_value(buf_parent_t, const void *) const;
};

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

void btree_erase_range_generic(value_sizer_t *sizer, key_tester_t *tester,
        const value_deleter_t *deleter, const btree_key_t *left_exclusive_or_null,
        const btree_key_t *right_inclusive_or_null, superblock_t *superblock,
        signal_t *interruptor, bool release_superblock = true,
        const std::function<void(const store_key_t &, const char *, const buf_parent_t &)>
            &on_erase_cb =
                std::function<void(const store_key_t &, const char *, const buf_parent_t &)>());

void erase_all(value_sizer_t *sizer,
               const value_deleter_t *deleter,
               superblock_t *superblock,
               signal_t *interruptor,
               bool release_superblock = true);

#endif  // BTREE_ERASE_RANGE_HPP_
