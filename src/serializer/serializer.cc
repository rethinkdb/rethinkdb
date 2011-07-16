#include <boost/bind.hpp>

#include "serializer/serializer.hpp"
#include "arch/coroutines.hpp"

// Blocking block_write implementation
boost::shared_ptr<serializer_t::block_token_t>
serializer_t::block_write(const void *buf, file_t::account_t *io_account) {
    // Default implementation: Wrap around non-blocking variant
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    boost::shared_ptr<block_token_t> result = block_write(buf, io_account, &cb);
    cb.wait();
    return result;
}

boost::shared_ptr<serializer_t::block_token_t>
serializer_t::block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account) {
    // Default implementation: Wrap around non-blocking variant
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    boost::shared_ptr<block_token_t> result = block_write(buf, block_id, io_account, &cb);
    cb.wait();
    return result;
}

// do_read implementation
static void do_read_wrapper(serializer_t *serializer, block_id_t block_id, void *buf,
                            file_t::account_t *io_account, serializer_t::read_callback_t *callback) {
    serializer->block_read(serializer->index_read(block_id), buf, io_account);
    callback->on_serializer_read();
}

bool serializer_t::do_read(block_id_t block_id, void *buf, file_t::account_t *io_account, read_callback_t *callback) {
    // Just a wrapper around the new interface. TODO: Get rid of this eventually
    coro_t::spawn(boost::bind(&do_read_wrapper, this, block_id, buf, io_account, callback));
    return false;
}

// do_write implementation
struct block_write_cond_t : public cond_t, public iocallback_t {
    block_write_cond_t(serializer_t::write_block_callback_t *cb) : callback(cb) { }
    void on_io_complete() {
        if (callback) callback->on_serializer_write_block();
        pulse();
    }
    serializer_t::write_block_callback_t *callback;
};

static void do_write_wrapper(serializer_t *serializer,
                             serializer_t::write_t *writes,
                             int num_writes,
                             file_t::account_t *io_account,
                             serializer_t::write_txn_callback_t *callback,
                             serializer_t::write_tid_callback_t *tid_callback)
{
    std::vector<block_write_cond_t*> block_write_conds;
    block_write_conds.reserve(num_writes);

    std::vector<serializer_t::index_write_op_t> index_write_ops;
    // Prepare a zero buf for deletions
    void *zerobuf = serializer->malloc();
    bzero(zerobuf, serializer->get_block_size().value());
    memcpy(zerobuf, "zero", 4); // TODO: This constant should be part of the serializer implementation or something like that or we should get rid of zero blocks completely...

    // Step 1: Write buffers to disk and assemble index operations
    for (size_t i = 0; i < (size_t)num_writes; ++i) {
        const serializer_t::write_t& write = writes[i];
        serializer_t::index_write_op_t op(write.block_id);
        if (write.recency_specified) op.recency = write.recency;

        // Buffer writes:
        if (write.buf_specified) {
            if (write.buf) {
                block_write_conds.push_back(new block_write_cond_t(write.callback));
                boost::shared_ptr<serializer_t::block_token_t> token = serializer->block_write(
                    write.buf, write.block_id, io_account, block_write_conds.back());

                // also generate the corresponding index op
                op.token = token;
                op.delete_bit = false;
                if (write.recency_specified) op.recency = write.recency;
            } else {
                // Deletion:
                boost::shared_ptr<serializer_t::block_token_t> token;
                if (write.write_empty_deleted_block) {
                    /* Extract might get confused if we delete a block, because it doesn't
                     * search the LBA for deletion entries. We clear things up for extract by
                     * writing a block with the block ID of the block to be deleted but zero
                     * contents. All that matters is that this block is on disk somewhere. */
                    block_write_conds.push_back(new block_write_cond_t(write.callback));
                    token = serializer->block_write(zerobuf, write.block_id, io_account, block_write_conds.back());
                }

                op.token = token;
                op.delete_bit = true;
            }
        } else { // Recency update
            rassert(write.recency_specified);
        }

        index_write_ops.push_back(op);
    }

    // Step 2: At the point where all writes have been started, we can call tid_callback
    if (tid_callback) {
        tid_callback->on_serializer_write_tid();
    }

    // Step 3: Wait on all writes to finish
    for (size_t i = 0; i < block_write_conds.size(); ++i) {
        block_write_conds[i]->wait();
        delete block_write_conds[i];
    }
    // (free the zerobuf)
    serializer->free(zerobuf);

    // Step 4: Commit the transaction to the serializer
    serializer->index_write(index_write_ops, io_account);
    index_write_ops.clear(); // clean up index_write_ops; not strictly necessary

    // Step 5: Call callback
    callback->on_serializer_write_txn();
}

bool serializer_t::do_write(write_t *writes, int num_writes, file_t::account_t *io_account,
                            write_txn_callback_t *callback, write_tid_callback_t *tid_callback)
{
    // Just a wrapper around the new interface.
    coro_t::spawn(boost::bind(&do_write_wrapper, this, writes, num_writes, io_account, callback, tid_callback));
    return false;
}
