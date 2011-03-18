#include "btree/delete_expired.hpp"

#include "btree/modify_oper.hpp"

class btree_delete_expired_oper_t : public btree_modify_oper_t
{
public:
    bool operate(UNUSED const boost::shared_ptr<transactor_t>& txor, UNUSED btree_value *old_value, UNUSED boost::scoped_ptr<large_buf_t>& old_large_buflock, UNUSED btree_value **new_value, UNUSED boost::scoped_ptr<large_buf_t>& new_large_buflock) {
        /* Don't do anything. run_btree_modify_oper() will take advantage of
         * the fact that we got to the leaf in write mode to automatically
         * delete the expired key if necessary. */
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

void co_btree_delete_expired(const store_key_t &key, btree_slice_t *slice) {
    btree_delete_expired_oper_t oper;
    run_btree_modify_oper(&oper, slice, key, castime_t(BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS, repli_timestamp::invalid));
}

void btree_delete_expired(const store_key_t &key, btree_slice_t *slice) {
    coro_t::spawn_now(boost::bind(co_btree_delete_expired, key, slice));
}
