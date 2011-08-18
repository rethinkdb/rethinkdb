#ifndef BTREE_ERASE_RANGE_HPP_
#define BTREE_ERASE_RANGE_HPP_

class btree_slice_t;
struct store_key_t;
class order_token_t;

void btree_erase_range(btree_slice_t *slice, int hash_value, int hashmod,
                       bool left_key_supplied, const store_key_t& left_key_exclusive,
                       bool right_key_supplied, const store_key_t& right_key_inclusive,
                       order_token_t token);


#endif  // BTREE_ERASE_RANGE_HPP_
