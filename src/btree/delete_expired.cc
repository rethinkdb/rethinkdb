#include "btree/delete_expired.hpp"

#include "btree/modify_oper.hpp"

class btree_delete_expired_oper_t : public btree_modify_oper_t
{
public:
    bool operate(transaction_t *txn, btree_value *old_value, large_buf_lock_t& old_large_buflock, btree_value **new_value, large_buf_lock_t& new_large_buflock) {
        /* Don't do anything. run_btree_modify_oper() will take advantage of
         * the fact that we got to the leaf in write mode to automatically
         * delete the expired key if necessary. */
        return false;
    }
};

// This function is called when doing a btree_get() and finding an expired key.
// However, the key isn't guaranteed to keep being valid existing until the
// delete_expired coroutine is actually spawned. So for now we'll make a copy
// of the key and then free it when delete_expired is done.

void co_btree_delete_expired(btree_key *key_copy, btree_slice_t *slice) {
    btree_delete_expired_oper_t oper;
    run_btree_modify_oper(&oper, slice, key_copy);
    free(key_copy);
}

void btree_delete_expired(const btree_key *key, btree_slice_t *slice) {
    btree_key *key_copy = (btree_key *) malloc(key->size + sizeof(btree_key));
    keycpy(key_copy, key);
    coro_t::spawn(co_btree_delete_expired, key_copy, slice);
}
