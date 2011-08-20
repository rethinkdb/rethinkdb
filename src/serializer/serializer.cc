#include <boost/bind.hpp>
#include <boost/variant.hpp>

#include "serializer/serializer.hpp"
#include "arch/arch.hpp"

file_account_t *serializer_t::make_io_account(int priority) {
    return make_io_account(priority, UNLIMITED_OUTSTANDING_REQUESTS);
}

serializer_t::index_write_op_t::index_write_op_t(
        block_id_t block_id,
        boost::optional<boost::shared_ptr<serializer_block_token_t> > token,
        boost::optional<repli_timestamp_t> recency,
        boost::optional<bool> delete_bit)
    : block_id(block_id), token(token), recency(recency), delete_bit(delete_bit) {}

void serializer_t::index_write(const index_write_op_t &op, file_account_t *io_account) {
    std::vector<index_write_op_t> ops;
    ops.push_back(op);
    return index_write(ops, io_account);
}

// Blocking block_read implementation
void serializer_t::block_read(const boost::shared_ptr<serializer_block_token_t>& token, void *buf, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    block_read(token, buf, io_account, &cb);
    cb.wait();
}

// Blocking block_write implementation
boost::shared_ptr<serializer_block_token_t>
serializer_t::block_write(const void *buf, file_account_t *io_account) {
    // Default implementation: Wrap around non-blocking variant
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    boost::shared_ptr<serializer_block_token_t> result = block_write(buf, io_account, &cb);
    cb.wait();
    return result;
}

boost::shared_ptr<serializer_block_token_t>
serializer_t::block_write(const void *buf, block_id_t block_id, file_account_t *io_account) {
    // Default implementation: Wrap around non-blocking variant
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    boost::shared_ptr<serializer_block_token_t> result = block_write(buf, block_id, io_account, &cb);
    cb.wait();
    return result;
}

boost::shared_ptr<serializer_block_token_t>
serializer_t::block_write(const void *buf, file_account_t *io_account, iocallback_t *cb) {
    return block_write(buf, NULL_BLOCK_ID, io_account, cb);
}

// do_write implementation
serializer_t::write_t::write_t(block_id_t block_id, action_t action) : block_id(block_id), action(action) {}

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

serializer_t::write_t serializer_t::write_t::make_delete(block_id_t block_id, bool write_zero_block) {
    delete_t del;
    del.write_zero_block = write_zero_block;
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
    void *zerobuf;
    std::vector<write_cond_t*> *block_write_conds;
    serializer_t::index_write_op_t *op;
    write_performer_t(serializer_t *ser, file_account_t *acct, void *zerobuf,
                      std::vector<write_cond_t*> *conds, serializer_t::index_write_op_t *op)
        : serializer(ser), io_account(acct), zerobuf(zerobuf), block_write_conds(conds), op(op) {}

    void operator()(const serializer_t::write_t::update_t &update) {
        block_write_conds->push_back(new write_cond_t(update.io_callback));
        op->token = serializer->block_write(update.buf, op->block_id, io_account, block_write_conds->back());
        if (update.launch_callback)
            update.launch_callback->on_write_launched(op->token.get());
        op->delete_bit = false;
        op->recency = update.recency;
    }

    void operator()(const serializer_t::write_t::delete_t &del) {
        op->token = boost::shared_ptr<serializer_block_token_t>();
        op->delete_bit = true;
        op->recency = repli_timestamp_t::invalid;
        if (del.write_zero_block) {
            block_write_conds->push_back(new write_cond_t(NULL));
            op->token = serializer->block_write(zerobuf, op->block_id, io_account, block_write_conds->back());
        }
    }

    void operator()(const serializer_t::write_t::touch_t &touch) {
        op->recency = touch.recency;
    }
};

void serializer_t::do_write(std::vector<write_t> writes, file_account_t *io_account) {
    std::vector<write_cond_t*> block_write_conds;
    std::vector<serializer_t::index_write_op_t> index_write_ops;
    block_write_conds.reserve(writes.size());
    index_write_ops.reserve(writes.size());

    // Prepare a zero buf for deletions.
    void *zerobuf = malloc();
    bzero(zerobuf, get_block_size().value());
    memcpy(zerobuf, "zero", 4); // TODO: This constant should be part of the serializer implementation or something like that or we should get rid of zero blocks completely...

    // Step 1: Write buffers to disk and assemble index operations
    for (size_t i = 0; i < writes.size(); ++i) {
        const serializer_t::write_t& write = writes[i];
        serializer_t::index_write_op_t op(write.block_id);
        write_performer_t performer(this, io_account, zerobuf, &block_write_conds, &op);
        boost::apply_visitor(performer, write.action);
        index_write_ops.push_back(op);
    }

    // Step 2: Wait on all writes to finish
    for (size_t i = 0; i < block_write_conds.size(); ++i) {
        block_write_conds[i]->wait();
        delete block_write_conds[i];
    }
    block_write_conds.clear();
    free(zerobuf);

    // Step 3: Commit the transaction to the serializer
    index_write(index_write_ops, io_account);
}
