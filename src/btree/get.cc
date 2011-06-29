#include <boost/shared_ptr.hpp>
#include "btree/delete_expired.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/btree_data_provider.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "store.hpp"

void acquire_and_swap_buf(buf_lock_t *buf, transaction_t *txn, block_id_t node_id, access_t access) {
    buf_lock_t tmp(txn, node_id, access);
    buf->swap(tmp);
}

get_result_t btree_get(const store_key_t &store_key, btree_slice_t *slice, order_token_t token) {
    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    // TODO: We should really already be on the right thread.
    on_thread_t mover(slice->home_thread());
    // We can use repli_timestamp::invalid here because it's the timestamp for a read-only transaction.
    slice->pre_begin_transaction_sink_.check_out(token);
    order_token_t begin_transaction_token = slice->pre_begin_transaction_read_mode_source_.check_in(token.tag() + "+begin_transaction_token").with_read_mode();
    transaction_t txn(slice->cache(), rwi_read);
    slice->post_begin_transaction_sink_.check_out(begin_transaction_token);

    txn.set_token(slice->post_begin_transaction_source_.check_in(token.tag() + "+post").with_read_mode());

    // Acquire the superblock
    buf_lock_t buf(&txn, SUPERBLOCK_ID, rwi_read);
    block_id_t node_id = reinterpret_cast<const btree_superblock_t *>(buf->get_data_read())->root_block;
    rassert(node_id != SUPERBLOCK_ID);

    if (node_id == NULL_BLOCK_ID) { // No root, so the tree is empty
        return get_result_t();
    }

    // Acquire the root
    acquire_and_swap_buf(&buf, &txn, node_id, rwi_read);
    DEBUG_ONLY(node::validate(slice->cache()->get_block_size(), reinterpret_cast<const node_t *>(buf->get_data_read())));

    // Go down the tree to the leaf
    while (node::is_internal(reinterpret_cast<const node_t *>(buf->get_data_read()))) {
        node_id = internal_node::lookup(reinterpret_cast<const internal_node_t *>(buf->get_data_read()), key);
        rassert(node_id != NULL_BLOCK_ID && node_id != SUPERBLOCK_ID);

        acquire_and_swap_buf(&buf, &txn, node_id, rwi_read);
        DEBUG_ONLY(node::validate(slice->cache()->get_block_size(), reinterpret_cast<const node_t *>(buf->get_data_read())));
    }

    // Got down to the leaf, now examine it
    const leaf_node_t *leaf = reinterpret_cast<const leaf_node_t *>(buf->get_data_read());
    int key_index = leaf::impl::find_key(leaf, key);

    if (key_index == leaf::impl::key_not_found) { // Key not found
        return get_result_t();
    }

    const btree_value_t *value = leaf::get_pair_by_index(leaf, key_index)->value();

    if (value->expired()) {
        // If the value is expired, delete it in the background
        btree_delete_expired(store_key, slice);
        return get_result_t();
    }

    /* Construct a data-provider to hold the result */
    boost::shared_ptr<value_data_provider_t> dp(value_data_provider_t::create(value, &txn));

    return get_result_t(dp, value->mcflags(), 0);
}
