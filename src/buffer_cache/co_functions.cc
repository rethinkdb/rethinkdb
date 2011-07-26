#include "buffer_cache/co_functions.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/promise.hpp"

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) {
    transaction->assert_thread();
    buf_t *value = transaction->acquire(block_id, mode, acquisition_cond ? boost::bind(&threadsafe_cond_t::pulse, acquisition_cond) : boost::function<void()>());
    rassert(value);
    return value;
}


void co_get_subtree_recencies(transaction_t *txn, block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out) {
    struct : public get_subtree_recencies_callback_t {
        void got_subtree_recencies() { cond.pulse(); }
        cond_t cond;
    } cb;

    txn->get_subtree_recencies(block_ids, num_block_ids, recencies_out, &cb);

    cb.cond.wait();
}
