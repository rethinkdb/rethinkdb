#include "btree/rget.hpp"

store_t::rget_result_t btree_rget(btree_key_value_store_t *store, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results) {
    return store_t::rget_result_t();
}

store_t::rget_result_t btree_rget_slice(btree_slice_t *slice, store_key_t *start, store_key_t *end, bool left_open, bool right_open, uint64_t max_results) {
    return store_t::rget_result_t();
}
