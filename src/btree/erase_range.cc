#include "btree/erase_range.hpp"

#include "concurrency/fifo_checker.hpp"
#include "errors.hpp"

void btree_erase_range(UNUSED btree_slice_t *slice, UNUSED int hash_value, UNUSED int hashmod,
                       UNUSED bool left_key_supplied, UNUSED const store_key_t& left_key_exclusive,
                       UNUSED bool right_key_supplied, UNUSED const store_key_t& right_key_inclusive,
                       UNUSED order_token_t token) {
    crash("not implemented");
}
