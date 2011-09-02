#include <boost/bind.hpp>
#include <boost/variant.hpp>

#include "serializer/serializer.hpp"
#include "arch/arch.hpp"

file_account_t *serializer_t::make_io_account(int priority) {
    return make_io_account(priority, UNLIMITED_OUTSTANDING_REQUESTS);
}

// Blocking block_read implementation
void serializer_t::block_read(const boost::intrusive_ptr<standard_block_token_t>& token, void *buf, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    block_read(token, buf, io_account, &cb);
    cb.wait();
}

// Blocking block_write implementation
boost::intrusive_ptr<standard_block_token_t>
serializer_t::block_write(const void *buf, file_account_t *io_account) {
    return serializer_block_write(this, buf, io_account);
}

boost::intrusive_ptr<standard_block_token_t>
serializer_t::block_write(const void *buf, block_id_t block_id, file_account_t *io_account) {
    return serializer_block_write(this, buf, block_id, io_account);
}

boost::intrusive_ptr<standard_block_token_t>
serializer_t::block_write(const void *buf, file_account_t *io_account, iocallback_t *cb) {
    return serializer_block_write(this, buf, io_account, cb);
}

// do_write implementation
serializer_t::write_t::write_t(block_id_t _block_id, action_t _action)
    : block_id(_block_id), action(_action) { }

serializer_t::write_t serializer_t::write_t::make_touch(block_id_t block_id, repli_timestamp_t recency) {
    touch_t touch;
    touch.recency = recency;
    return write_t(block_id, touch);
}

serializer_t::write_t serializer_t::write_t::make_update(
        block_id_t block_id, repli_timestamp_t recency, const void *buf,
        iocallback_t *io_callback, write_launched_callback_t *launch_callback)
{
    update_t update;
    update.buf = buf, update.recency = recency, update.io_callback = io_callback,
        update.launch_callback = launch_callback;
    return write_t(block_id, update);
}

serializer_t::write_t serializer_t::write_t::make_delete(block_id_t block_id) {
    delete_t del;
    return write_t(block_id, del);
}

struct write_cond_t : public cond_t, public iocallback_t {
    write_cond_t(iocallback_t *cb) : callback(cb) { }
    void on_io_complete() {
        if (callback)
            callback->on_io_complete();
        pulse();
    }
    iocallback_t *callback;
};

struct write_performer_t : public boost::static_visitor<void> {
    serializer_t *serializer;
    file_account_t *io_account;
    std::vector<write_cond_t*> *block_write_conds;
    index_write_op_t *op;
    write_performer_t(serializer_t *ser, file_account_t *acct,
                      std::vector<write_cond_t*> *conds, index_write_op_t *_op)
        : serializer(ser), io_account(acct), block_write_conds(conds), op(_op) {}

    void operator()(const serializer_t::write_t::update_t &update) {
        block_write_conds->push_back(new write_cond_t(update.io_callback));
        op->token = serializer->block_write(update.buf, op->block_id, io_account, block_write_conds->back());
        if (update.launch_callback)
            update.launch_callback->on_write_launched(op->token.get());
        op->recency = update.recency;
    }

    void operator()(UNUSED const serializer_t::write_t::delete_t &del) {
        op->token = boost::intrusive_ptr<standard_block_token_t>();
        op->recency = repli_timestamp_t::invalid;
    }

    void operator()(const serializer_t::write_t::touch_t &touch) {
        op->recency = touch.recency;
    }
};

void serializer_t::do_write(const std::vector<write_t>& writes, file_account_t *io_account) {
    std::vector<write_cond_t*> block_write_conds;
    std::vector<index_write_op_t> index_write_ops;
    block_write_conds.reserve(writes.size());
    index_write_ops.reserve(writes.size());

    // Step 1: Write buffers to disk and assemble index operations
    for (size_t i = 0; i < writes.size(); ++i) {
        const serializer_t::write_t& write = writes[i];
        index_write_op_t op(write.block_id);
        write_performer_t performer(this, io_account, &block_write_conds, &op);
        boost::apply_visitor(performer, write.action);
        index_write_ops.push_back(op);
    }

    // Step 2: Wait on all writes to finish
    for (size_t i = 0; i < block_write_conds.size(); ++i) {
        block_write_conds[i]->wait();
        delete block_write_conds[i];
    }
    block_write_conds.clear();

    // Step 3: Commit the transaction to the serializer
    index_write(index_write_ops, io_account);
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

void serializer_data_ptr_t::init_clone(serializer_t *ser, serializer_data_ptr_t& other) {
    rassert(other.ptr_);
    rassert(!ptr_);
    ptr_ = ser->clone(other.ptr_);
}
