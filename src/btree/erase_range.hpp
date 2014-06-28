// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BTREE_ERASE_RANGE_HPP_
#define BTREE_ERASE_RANGE_HPP_

#include <functional>

#include "errors.hpp"
#include "btree/node.hpp"
#include "btree/types.hpp"
#include "buffer_cache/types.hpp"

class buf_parent_t;
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

/* The return value of the `on_erase_cb` determines whether this function
continues erasing more values (true) or not (false). */
void btree_erase_range_generic(value_sizer_t *sizer, key_tester_t *tester,
        const value_deleter_t *deleter, const btree_key_t *left_exclusive_or_null,
        const btree_key_t *right_inclusive_or_null, superblock_t *superblock,
        signal_t *interruptor,
        release_superblock_t release_superblock = release_superblock_t::RELEASE,
        const std::function<void(const store_key_t &, const char *, const buf_parent_t &)>
            &on_erase_cb = std::function<void(const store_key_t &, const char *,
                                              const buf_parent_t &)>());

#endif  // BTREE_ERASE_RANGE_HPP_
