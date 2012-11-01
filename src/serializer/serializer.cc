// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/serializer.hpp"
#include "arch/arch.hpp"

file_account_t *serializer_t::make_io_account(int priority) {
    assert_thread();
    return make_io_account(priority, UNLIMITED_OUTSTANDING_REQUESTS);
}

// Blocking block_read implementation
void serializer_t::block_read(const intrusive_ptr_t<standard_block_token_t>& token, void *buf, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    block_read(token, buf, io_account, &cb);
    cb.wait();
}

serializer_write_t serializer_write_t::make_touch(block_id_t block_id, repli_timestamp_t recency) {
    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = TOUCH;
    w.action.touch.recency = recency;
    return w;
}

serializer_write_t serializer_write_t::make_update(
        block_id_t block_id, repli_timestamp_t recency, const void *buf,
        iocallback_t *io_callback, serializer_write_launched_callback_t *launch_callback)
{
    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = UPDATE;
    w.action.update.buf = buf;
    w.action.update.recency = recency;
    w.action.update.io_callback = io_callback;
    w.action.update.launch_callback = launch_callback;
    return w;
}

serializer_write_t serializer_write_t::make_delete(block_id_t block_id) {
    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = DELETE;
    return w;
}

struct write_cond_t : public cond_t, public iocallback_t {
    explicit write_cond_t(iocallback_t *cb) : callback(cb) { }
    void on_io_complete() {
        if (callback)
            callback->on_io_complete();
        pulse();
    }
    iocallback_t *callback;
};

void perform_write(const serializer_write_t *write, serializer_t *ser, serializer_transaction_t *ser_txn,
                   file_account_t *acct, std::vector<write_cond_t *> *conds, index_write_op_t *op) {
    switch (write->action_type) {
    case serializer_write_t::UPDATE: {
        conds->push_back(new write_cond_t(write->action.update.io_callback));
        op->token = ser->block_write(ser_txn, write->action.update.buf, op->block_id, acct, conds->back());
        if (write->action.update.launch_callback) {
            write->action.update.launch_callback->on_write_launched(op->token.get());
        }
        op->recency = write->action.update.recency;
    } break;
    case serializer_write_t::DELETE: {
        op->token = intrusive_ptr_t<standard_block_token_t>();
        op->recency = repli_timestamp_t::invalid;
    } break;
    case serializer_write_t::TOUCH: {
        op->recency = write->action.touch.recency;
    } break;
    default:
        unreachable();
    }
}

void do_writes(serializer_t *ser, const std::vector<serializer_write_t>& writes, file_account_t *io_account) {
    ser->assert_thread();
    serializer_transaction_t ser_txn;
    std::vector<write_cond_t*> block_write_conds;
    std::vector<index_write_op_t> index_write_ops;
    block_write_conds.reserve(writes.size());
    index_write_ops.reserve(writes.size());

    // Step 1: Write buffers to disk and assemble index operations
    for (size_t i = 0; i < writes.size(); ++i) {
        const serializer_write_t *write = &writes[i];
        index_write_op_t op(write->block_id);

        perform_write(write, &ser_txn, io_account, &block_write_conds, &op);

        index_write_ops.push_back(op);
    }

    // Step 2: Wait on all writes to finish
    for (size_t i = 0; i < block_write_conds.size(); ++i) {
        block_write_conds[i]->wait();
        delete block_write_conds[i];
    }
    block_write_conds.clear();

    // Step 3: Commit the transaction to the serializer
    ser->index_write(&ser_txn, index_write_ops, io_account);
}

void serializer_data_ptr_t::free(serializer_t *ser) {
    rassert(ptr_);
    ser->free(ptr_);
    ptr_ = NULL;
}

void serializer_data_ptr_t::init_malloc(serializer_t *ser) {
    rassert(!ptr_);
    ptr_ = ser->malloc();
}

void serializer_data_ptr_t::init_clone(serializer_t *ser, const serializer_data_ptr_t& other) {
    rassert(other.ptr_);
    rassert(!ptr_);
    ptr_ = ser->clone(other.ptr_);
}
