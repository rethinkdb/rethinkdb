#include "btree/modify_oper.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "btree/internal_node.hpp"
#include "btree/leaf_node.hpp"
#include "btree/operations.hpp"
#include "btree/slice.hpp"


void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &store_key, castime_t castime, order_token_t token) {
    btree_key_buffer_t kbuffer(store_key);
    btree_key_t *key = kbuffer.key();

    block_size_t block_size = slice->cache()->get_block_size();

    {
        boost::scoped_ptr<transaction_t> txn;
        got_superblock_t got_superblock;

        get_btree_superblock(slice, rwi_write, oper->compute_expected_change_count(block_size), castime.timestamp, token, &got_superblock, txn);

        keyvalue_location_t<memcached_value_t> kv_location;
        find_keyvalue_location_for_write(txn.get(), &got_superblock, key, &kv_location);
        scoped_malloc<memcached_value_t> the_value;
        the_value.reinterpret_swap(kv_location.value);

        bool expired = the_value && the_value->expired();

        // If the value's expired, delete it.
        if (expired) {
            blob_t b(the_value->value_ref(), blob::btree_maxreflen);
            b.clear(txn.get());
            the_value.reset();
        }

        bool update_needed = oper->operate(txn.get(), the_value);
        update_needed = update_needed || expired;

        // Add a CAS to the value if necessary
        if (the_value) {
            if (the_value->has_cas()) {
                rassert(castime.proposed_cas != BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS);
                the_value->set_cas(block_size, castime.proposed_cas);
            }
        }

        // Actually update the leaf, if needed.
        if (update_needed) {
            kv_location.value.reinterpret_swap(the_value);
            null_key_modification_callback_t<memcached_value_t> null_cb;
            apply_keyvalue_change(txn.get(), &kv_location, key, castime.timestamp, expired, &null_cb);
        }
    }
}
