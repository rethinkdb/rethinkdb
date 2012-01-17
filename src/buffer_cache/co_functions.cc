#include "buffer_cache/co_functions.hpp"
#include "buffer_cache/sequence_group.hpp"
#include "concurrency/promise.hpp"

struct co_block_available_callback_t : public block_available_callback_t {
    coro_t *self;
    buf_t *value;

    virtual void on_block_available(buf_t *block) {
        value = block;
        self->notify();
    }

    buf_t *join() {
        self = coro_t::self();
        coro_t::wait();
        return value;
    }
};

buf_t *co_acquire_block(transaction_t *transaction, block_id_t block_id, access_t mode, threadsafe_cond_t *acquisition_cond) {
    transaction->assert_thread();
    co_block_available_callback_t cb;
    buf_t *value = transaction->acquire(block_id, mode, &cb);
    if (acquisition_cond) {
        // TODO: This is worthless crap, because the
        // transaction_t::acquire interface is a lie.
        acquisition_cond->pulse();
    }
    if (!value) {
        value = cb.join();
    }
    rassert(value);
    return value;
}


struct large_value_acquired_t : public large_buf_available_callback_t {
    coro_t *self;
    large_value_acquired_t() : self(coro_t::self()) { }
    void on_large_buf_available(UNUSED large_buf_t *large_value) { self->notify(); }
};

void co_acquire_large_buf_for_unprepend(large_buf_t *lb, int64_t length) {
    large_value_acquired_t acquired;
    lb->assert_thread();
    lb->acquire_for_unprepend(length, &acquired);
    coro_t::wait();
}

void co_acquire_large_buf_slice(large_buf_t *lb, int64_t offset, int64_t size, threadsafe_cond_t *acquisition_cond) {
    large_value_acquired_t acquired;
    lb->assert_thread();
    lb->acquire_slice(offset, size, &acquired);
    if (acquisition_cond) {
        // TODO: This is worthless crap, because the
        // transaction_t::acquire interface is a lie.
        acquisition_cond->pulse();
    }
    coro_t::wait();
}

void co_acquire_large_buf(large_buf_t *lb, threadsafe_cond_t *acquisition_cond) {
    lb->assert_thread();
    co_acquire_large_buf_slice(lb, 0, lb->root_ref->size, acquisition_cond);
}

void co_acquire_large_buf_lhs(large_buf_t *lb) {
    lb->assert_thread();
    co_acquire_large_buf_slice(lb, 0, std::min(1L, lb->root_ref->size));
}

void co_acquire_large_buf_rhs(large_buf_t *lb) {
    lb->assert_thread();
    int64_t beg = std::max(int64_t(0), lb->root_ref->size - 1);
    co_acquire_large_buf_slice(lb, beg, lb->root_ref->size - beg);
}

void co_acquire_large_buf_for_delete(large_buf_t *large_value) {
    large_value_acquired_t acquired;
    large_value->acquire_for_delete(&acquired);
    coro_t::wait();
}

// Well this is repetitive.
struct transaction_begun_callback_t : public transaction_begin_callback_t {
    flat_promise_t<transaction_t*> txn;

    void on_txn_begin(transaction_t *txn_) {
        txn.pulse(txn_);
    }

    transaction_t *join() {
        return txn.wait();
    }
};

transaction_t *co_begin_transaction(cache_t *cache, sequence_group_t *seq_group, access_t access, int expected_change_count, repli_timestamp recency_timestamp) {
    cache->assert_thread();
    transaction_begun_callback_t cb;

    // HISTORICAL COMMENTS:

    // Writes and reads must separately have their order preserved,
    // but we're expecting, in the case of throttling, for writes to
    // be throttled while reads are not.

    // We think there is a bug in throttling that could reorder writes
    // when throttling _ends_.  Reordering can also happen if
    // begin_transaction sometimes returns NULL and other times does
    // not, because then in one case cb.join() needs to spin around
    // the event loop.  So there's an obvious problem with the code
    // below, if we did not have coro_fifo_acq_t protecting it.

    // MODERN COMMENTS:

    // Reads are now coro-fifoed in the same path that writes are.
    // This is dumb because we really just want reads coming from the
    // same _connection_ to be coro-fifoed in the same path that
    // writes are.

    // Why do "writes" need to have their order preserved at all, now
    // that we have sequence_group_t?  Perhaps it's because in the
    // presence of replication, that prevents the gated store gate
    // limitations being bypassed.  You could look at the gated store
    // code to prove or disprove this claim, but since v1.1.x is
    // supposed to be stable, we're being conservative.

    // TODO FIFO: Fix this as the above comment says.

    // It is important that we leave the sequence group's fifo _after_
    // we leave the write throttle fifo.  So we construct it first.
    // (The destructor will run after.)
    coro_fifo_acq_t seq_group_acq;
    // TODO FIFO: This is such a hack, calling ->serializer->get_mod_id
    seq_group_acq.enter(&seq_group->slice_groups[cache->get_mod_id()].fifo);

    coro_fifo_acq_t write_throttle_acq;

    if (is_write_mode(access)) {
        write_throttle_acq.enter(&cache->co_begin_coro_fifo());
    }

    transaction_t *value = cache->begin_transaction(access, expected_change_count, recency_timestamp, &cb);
    if (!value) {
        value = cb.join();
    }

    rassert(value);
    return value;
}



struct transaction_committed_t : public transaction_commit_callback_t {
    coro_t *self;
    transaction_committed_t() : self(coro_t::self()) { }
    void on_txn_commit(UNUSED transaction_t *transaction) {
        self->notify();
    }
};


void co_commit_transaction(transaction_t *transaction) {
    transaction->assert_thread();
    transaction_committed_t cb;
    if (!transaction->commit(&cb)) {
        coro_t::wait();
    }
}


void co_get_subtree_recencies(transaction_t *txn, block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out) {
    struct : public get_subtree_recencies_callback_t {
        void got_subtree_recencies() { cond.pulse(); }
        cond_t cond;
    } cb;

    txn->get_subtree_recencies(block_ids, num_block_ids, recencies_out, &cb);

    cb.cond.wait();
}
