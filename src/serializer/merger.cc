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

void merger_serializer_t::index_write(new_mutex_in_line_t *mutex_acq,
                                      const std::vector<index_write_op_t> &write_ops) {
    rassert(coro_t::self() != NULL);
    assert_thread();

    // Apply our set of write ops atomically
    for (auto op = write_ops.begin(); op != write_ops.end(); ++op) {
        push_index_write_op(*op);
    }

    // The caller is definitely "in line" for this merger serializer -- subsequent
    // index_write calls will get logically committed after ours.
    mutex_acq->reset();

    // Wait for the write to complete
    write_committer.sync();
}

void merger_serializer_t::do_index_write() {
    assert_thread();

    // Assemble the currently outstanding index writes into
    // a vector of index_write_op_t-s.
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
    }

    new_mutex_in_line_t mutex_acq(&inner_index_write_mutex);
    mutex_acq.acq_signal()->wait();
    inner->index_write(&mutex_acq, write_ops);
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

