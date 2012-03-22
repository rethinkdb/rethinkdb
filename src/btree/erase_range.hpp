#ifndef BTREE_ERASE_RANGE_HPP_
#define BTREE_ERASE_RANGE_HPP_

#include "errors.hpp"

class btree_slice_t;
struct store_key_t;
struct btree_key_t;
class order_token_t;
class sequence_group_t;

class key_tester_t {
public:
    key_tester_t() { }
    virtual bool key_should_be_erased(const btree_key_t *key) = 0;

protected:
    virtual ~key_tester_t() { }
private:
    DISABLE_COPYING(key_tester_t);
};

void btree_erase_range(btree_slice_t *slice, sequence_group_t *seq_group, key_tester_t *tester,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       order_token_t token);


#endif  // BTREE_ERASE_RANGE_HPP_
