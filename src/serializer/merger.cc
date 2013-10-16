// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "serializer/merger.hpp"

#include "errors.hpp"

#include "serializer/types.hpp"
#include "arch/runtime/coroutines.hpp"
#include "config/args.hpp"



merger_serializer_t::merger_serializer_t(serializer_t *_inner, int _max_active_writes) :
    inner(_inner),
    index_writes_io_account(make_io_account(MERGED_INDEX_WRITE_IO_PRIORITY)),
    num_active_writes(0),
    max_active_writes(_max_active_writes) {
}

merger_serializer_t::~merger_serializer_t() {
    assert_thread();
    rassert(num_active_writes == 0);
    rassert(outstanding_index_writes.empty());
    
    delete index_writes_io_account;
    delete inner;
}

void merger_serializer_t::index_write(const std::vector<index_write_op_t>& write_ops, file_account_t *) {
    rassert(coro_t::self() != NULL);
    assert_thread();
    
    std::shared_ptr<cond_t> write_complete;
    {
        // Our set of write ops must be processed atomically...
        ASSERT_NO_CORO_WAITING;
        for (auto op = write_ops.begin(); op != write_ops.end(); ++op) {
            push_index_write(*op);
        }
        // ... and we also take a copy of the on_index_writes_complete signal
        // so we get notified exactly when all of our write ops have
        // been completed.
        write_complete = on_index_writes_complete;
    }
    
    // Check if we can initiate a new index write
    if (num_active_writes < max_active_writes) {
        do_index_write();
    }
    
    // Wait for the write to complete
    write_complete->wait_lazily_unordered();
}

void merger_serializer_t::do_index_write() {
    assert_thread();
    rassert(num_active_writes < max_active_writes);
    
    ++num_active_writes;
    
    // Assemble the currently outstanding index writes into
    // a vector of index_write_op_t-s.
    std::shared_ptr<cond_t> write_complete;
    std::vector<index_write_op_t> write_ops;
    write_ops.reserve(outstanding_index_writes.size());
    {
        ASSERT_NO_CORO_WAITING;
        for (auto op_pair = outstanding_index_writes.begin(); op_pair != outstanding_index_writes.end(); ++op_pair) {
            write_ops.push_back(op_pair->second);
        }
        outstanding_index_writes.clear();
        
        // Swap out the on_index_writes_complete signal so subsequent index
        // writes can be captured by the next round of do_index_write().
        write_complete = on_index_writes_complete;
        on_index_writes_complete.reset(new cond_t());
    }
    
    inner->index_write(write_ops, index_writes_io_account);
    
    write_complete->pulse();
    
    --num_active_writes;
    
    // Check if we should start another index write
    if (num_active_writes < max_active_writes && !outstanding_index_writes.empty()) {
        coro_t::spawn_sometime(std::bind(&merger_serializer_t::do_index_write, this));
    }
}

void merger_serializer_t::merge_index_write(const index_write_op_t &to_be_merged, index_write_op_t *into_out) const {
    rassert(to_be_merged.block_id == into_out->block_id);
    if (to_be_merged.token.is_initialized()) {
        into_out->token = to_be_merged.token;
    }
    if (to_be_merged.recency.is_initialized()) {
        into_out->recency = to_be_merged.recency;
    }
}

void merger_serializer_t::push_index_write(const index_write_op_t &op) {
    auto existing_it = outstanding_index_writes.find(op.block_id);
    if (existing_it != outstanding_index_writes.end()) {
        merge_index_write(op, &existing_it->second);
    } else {
        outstanding_index_writes.insert(std::pair<block_id_t, index_write_op_t>(op.block_id, op));
    }
}

