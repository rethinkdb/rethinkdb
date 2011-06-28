#include "btree/delete_expired.hpp"

#include "btree/modify_oper.hpp"

class btree_delete_expired_oper_t : public btree_modify_oper_t
{
public:
    bool operate(UNUSED const boost::shared_ptr<transaction_t>& txn, UNUSED scoped_malloc<btree_value_t>& value) {
        /* Don't do anything.  run_btree_modify_oper() will take
           advantage of the fact that we got to the leaf in write mode
           to automatically delete the expired key if necessary. */
        return false;
    }

    int compute_expected_change_count(UNUSED const size_t block_size) {
        return 1;
    }
};

// This function is called when doing a btree_get() and finding an expired key.
// However, the key isn't guaranteed to keep being valid existing until the
// delete_expired coroutine is actually spawned. So for now we'll make a copy
// of the key and then free it when delete_expired is done.

// TODO: delete_expired (and for that matter, delete) should not
// really supply a timestamp, since it does not create a new key in
// the database, and so the subtrees it passes through have no data to
// replicate.
void co_btree_delete_expired(const store_key_t &key, btree_slice_t *slice) {
    btree_delete_expired_oper_t oper;
    // TODO: Something is wrong with our abstraction since we are
    // passing a completely meaningless proposed cas and because we
    // should not really be passing a recency timestamp.
    // It's okay to use repli_timestamp::invalid here.
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, repli_timestamp::invalid), order_token_t::ignore);
}

void btree_delete_expired(const store_key_t &key, btree_slice_t *slice) {
    coro_t::spawn_now(boost::bind(co_btree_delete_expired, key, slice));
}
