// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/merger.hpp"

#include "errors.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/new_mutex.hpp"
#include "config/args.hpp"
#include "serializer/types.hpp"



merger_serializer_t::merger_serializer_t(scoped_ptr_t<serializer_t> _inner,
                                         int _max_active_writes) :
    inner(std::move(_inner)),
    block_writes_io_account(make_io_account(MERGER_BLOCK_WRITE_IO_PRIORITY)),
    on_inner_index_write_complete(new counted_cond_t()),
    unhandled_index_write_waiter_exists(false),
    num_active_writes(0),
    max_active_writes(_max_active_writes) {
}

merger_serializer_t::~merger_serializer_t() {
    assert_thread();
    rassert(num_active_writes == 0);
    rassert(outstanding_index_write_ops.empty());
}

void merger_serializer_t::index_write(new_mutex_in_line_t *mutex_acq,
                                      const std::vector<index_write_op_t> &write_ops) {
    rassert(coro_t::self() != NULL);
    assert_thread();

    counted_t<counted_cond_t> write_complete;
    {
        // Our set of write ops must be processed atomically...
        ASSERT_NO_CORO_WAITING;
        for (auto op = write_ops.begin(); op != write_ops.end(); ++op) {
            push_index_write_op(*op);
        }
        // ... and we also take a copy of the on_inner_index_write_complete signal
        // so we get notified exactly when all of our write ops have
        // been completed.
        write_complete = on_inner_index_write_complete;
        unhandled_index_write_waiter_exists = true;
    }

    // The caller is definitely "in line" for this merger serializer -- subsequent
    // index_write calls will get logically committed after ours.
    mutex_acq->reset();

    // Check if we can initiate a new index write
    if (num_active_writes < max_active_writes) {
        ++num_active_writes;
        do_index_write();
    }

    // Wait for the write to complete
    write_complete->wait_lazily_unordered();
}

void merger_serializer_t::do_index_write() {
    assert_thread();
    rassert(num_active_writes <= max_active_writes);

    // Assemble the currently outstanding index writes into
    // a vector of index_write_op_t-s.
    counted_t<counted_cond_t> write_complete;
    std::vector<index_write_op_t> write_ops;
    write_ops.reserve(outstanding_index_write_ops.size());
    {
        ASSERT_NO_CORO_WAITING;
        for (auto op_pair = outstanding_index_write_ops.begin();
             op_pair != outstanding_index_write_ops.end();
             ++op_pair) {
            write_ops.push_back(op_pair->second);
        }
        outstanding_index_write_ops.clear();
        unhandled_index_write_waiter_exists = false;

        // Swap out the on_inner_index_write_complete signal so subsequent index
        // writes can be captured by the next round of do_index_write().
        write_complete.reset(new counted_cond_t());
        write_complete.swap(on_inner_index_write_complete);
    }

    new_mutex_in_line_t mutex_acq(&inner_index_write_mutex);
    mutex_acq.acq_signal()->wait();
    inner->index_write(&mutex_acq, write_ops);

    write_complete->pulse();

    --num_active_writes;

    // Check if we should start another index write
    if (num_active_writes < max_active_writes
        && unhandled_index_write_waiter_exists) {
        ++num_active_writes;
        coro_t::spawn_sometime(std::bind(&merger_serializer_t::do_index_write, this));
    }
}

void merger_serializer_t::merge_index_write_op(const index_write_op_t &to_be_merged,
                                               index_write_op_t *into_out) const {
    rassert(to_be_merged.block_id == into_out->block_id);
    if (to_be_merged.token.is_initialized()) {
        into_out->token = to_be_merged.token;
    }
    if (to_be_merged.recency.is_initialized()) {
        into_out->recency = to_be_merged.recency;
    }
}

void merger_serializer_t::push_index_write_op(const index_write_op_t &op) {
    auto existing_pair =
        outstanding_index_write_ops.insert(
            std::pair<block_id_t, index_write_op_t>(op.block_id, op));

    if (!existing_pair.second) {
        // new op could not be inserted because it already exists. Merge instead.
        merge_index_write_op(op, &existing_pair.first->second);
    }
}

