#include "redis/redis_types.hpp"

#include "buffer_cache/buffer_cache.hpp"
#include "redis/counted/counted.hpp"

void redis_list_value_t::clear(transaction_t *txn) {
    counted_btree_t bt(get_ref(), txn->get_cache()->get_block_size(), txn);
    bt.clear();
}

void redis_sorted_set_value_t::clear(transaction_t *txn) {
    // TODO clear member index

    sub_ref_t ref;
    ref.count = get_sub_size();
    ref.node_id = get_score_index_root();
    counted_btree_t bt(&ref, txn->get_cache()->get_block_size(), txn);
    bt.clear();
    get_sub_size() = 0;
    get_score_index_root() = NULL_BLOCK_ID;
}
