// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/merger.hpp"

#include <functional>

#include "errors.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/new_mutex.hpp"
#include "config/args.hpp"
#include "serializer/types.hpp"


merger_serializer_t::merger_serializer_t(scoped_ptr_t<serializer_t> _inner,
                                         int _max_active_writes) :
    inner(std::move(_inner)),
    block_writes_io_account(make_io_account(MERGER_BLOCK_WRITE_IO_PRIORITY)),
    write_committer(std::bind(&merger_serializer_t::do_index_write, this),
                    _max_active_writes) { }

merger_serializer_t::~merger_serializer_t() {
    assert_thread();
    rassert(outstanding_index_write_ops.empty());
}

counted_t<standard_block_token_t> merger_serializer_t::index_read(block_id_t block_id) {
    // First check if there is an updated entry for the block id...
    const auto write_op = outstanding_index_write_ops.find(block_id);
    if (write_op != outstanding_index_write_ops.end()) {
        if (static_cast<bool>(write_op->second.token)) {
            return *write_op->second.token;
        }
    }

    // ... otherwise pass this request on to the underlying serializer.
    return inner->index_read(block_id);
}

void merger_serializer_t::index_write(new_mutex_in_line_t *mutex_acq,
                                      const std::function<void()> &on_writes_reflected,
                                      const std::vector<index_write_op_t> &write_ops) {
    rassert(coro_t::self() != NULL);
    assert_thread();

    // Apply our set of write ops atomically
    {
        new_mutex_acq_t outstanding_mutex_acq(&outstanding_index_write_mutex);
        for (auto op = write_ops.begin(); op != write_ops.end(); ++op) {
            push_index_write_op(*op);
        }
    }

    // Changes are now visible for subsequent `index_read()` calls.
    on_writes_reflected();

    // The caller is "in line" for this merger serializer -- subsequent index_write
    // calls will get logically committed after ours, and subsequent index_read
    // calls are going to see the changes.
    mutex_acq->reset();

    // Wait for the write to complete
    write_committer.notify();
    cond_t non_interruptor;
    write_committer.flush(&non_interruptor);
}

void merger_serializer_t::do_index_write() {
    assert_thread();

    // Pause changes to outstanding_index_write_ops
    new_mutex_in_line_t outstanding_mutex_acq(&outstanding_index_write_mutex);
    outstanding_mutex_acq.acq_signal()->wait_lazily_unordered();

    // Assemble the currently outstanding index writes into
    // a vector of index_write_op_t-s.
    std::vector<index_write_op_t> write_ops;
    write_ops.reserve(outstanding_index_write_ops.size());
    for (auto op_pair = outstanding_index_write_ops.begin();
         op_pair != outstanding_index_write_ops.end();
         ++op_pair) {
        write_ops.push_back(op_pair->second);
    }

    new_mutex_in_line_t mutex_acq(&inner_index_write_mutex);
    mutex_acq.acq_signal()->wait();
    inner->index_write(
        &mutex_acq,
        [&]() {
            // Once the writes are reflected in future calls to `inner->index_read()`,
            // we can reset outstanding_index_write_ops and allow new write ops to
            // get in line.
            outstanding_index_write_ops.clear();
            outstanding_mutex_acq.reset();
        },
        write_ops);
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

