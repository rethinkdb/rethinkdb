// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_MERGER_HPP_
#define SERIALIZER_MERGER_HPP_

#include <vector>
#include <map>
#include <memory>

#include "buffer_cache/types.hpp"
#include "concurrency/new_mutex.hpp"
#include "containers/scoped.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/serializer.hpp"

/*
 * The merger serializer is a wrapper around another serializer. It limits
 * the number of active index_writes. If more index_writes come in while
 * `max_active_writes` index_writes are already going on, the new index
 * writes are queued up.
 * The advantage of this is that multiple index writes (e.g. coming from different
 * hash shards) can be merged together, improving efficiency and significantly
 * reducing the number of disk seeks on rotational drives.
 *
 * As an additional optimization, merger_serializer_t uses a common file account
 * for all block_writes, so reduce the amount of random disk seeks that can
 * occur when writes from multiple different accounts get interleaved (see
 * https://github.com/rethinkdb/rethinkdb/issues/3348 )
 */

class merger_serializer_t : public serializer_t {
public:
    merger_serializer_t(scoped_ptr_t<serializer_t> _inner, int _max_active_writes);
    ~merger_serializer_t();


    /* Allocates a new io account for the underlying file.
    Use delete to free it. */
    using serializer_t::make_io_account;
    file_account_t *make_io_account(int priority, int outstanding_requests_limit) {
        return inner->make_io_account(priority, outstanding_requests_limit);
    }

    /* Some serializer implementations support read-ahead to speed up cache warmup.
    This is supported through a serializer_read_ahead_callback_t which gets called
    whenever the serializer has read-ahead some buf.  The callee can then decide
    whether it wants to use the offered buffer of discard it.
    */
    void register_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
        inner->register_read_ahead_cb(cb);
    }
    void unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb) {
        inner->unregister_read_ahead_cb(cb);
    }

    // Reading a block from the serializer.  Reads a block, blocks the coroutine.
    buf_ptr_t block_read(const counted_t<standard_block_token_t> &token,
                       file_account_t *io_account) {
        return inner->block_read(token, io_account);
    }

    /* The index stores three pieces of information for each ID:
     * 1. A pointer to a data block on disk (which may be NULL)
     * 2. A repli_timestamp_t, called the "recency"
     * 3. A boolean, called the "delete bit" */

    /* max_block_id() and get_delete_bit() are used by the buffer cache to
    reconstruct the free list of unused block IDs. */

    /* Returns a block ID such that every existing block has an ID less than
     * that ID. Note that index_read(max_block_id() - 1) is not guaranteed to be
     * non-NULL. Note that for k > 0, max_block_id() - k might have never been
     * created. */
    block_id_t max_block_id() { return inner->max_block_id(); }

    segmented_vector_t<repli_timestamp_t> get_all_recencies(block_id_t first,
                                                            block_id_t step) {
        return inner->get_all_recencies(first, step);
    }

    /* Reads the block's delete bit. */
    bool get_delete_bit(block_id_t id) { return inner->get_delete_bit(id); }

    /* Reads the block's actual data */
    counted_t<standard_block_token_t> index_read(block_id_t block_id) {
        return inner->index_read(block_id);
    }

    /* index_write() applies all given index operations in an atomic way */
    /* This is where merger_serializer_t merges operations */
    void index_write(new_mutex_in_line_t *mutex_acq,
                     const std::vector<index_write_op_t> &write_ops);

    // Returns block tokens in the same order as write_infos.
    std::vector<counted_t<standard_block_token_t> >
    block_writes(const std::vector<buf_write_info_t> &write_infos,
                 UNUSED file_account_t *io_account,
                 iocallback_t *cb) {
        // Currently, we do not merge block writes, only index writes.
        // However we do use a common file account for all of them, which
        // reduces random disk seeks that would arise from trying to interleave
        // writes from the individual accounts further down in the i/o layer.
        return inner->block_writes(write_infos, block_writes_io_account.get(), cb);
    }

    /* The size, in bytes, of each serializer block */
    max_block_size_t max_block_size() const { return inner->max_block_size(); }

    /* Return true if no other processes have the file locked */
    bool coop_lock_and_check() { return inner->coop_lock_and_check(); }

    /* Return true if the garbage collector is active */
    bool is_gc_active() const {
        return inner->is_gc_active();
    }

private:
    // Adds `op` to `outstanding_index_write_ops`, using `merge_index_write_op()` if
    // necessary
    void push_index_write_op(const index_write_op_t &op);
    // This merges to_be_merged in-place into into_out.
    void merge_index_write_op(const index_write_op_t &to_be_merged,
                              index_write_op_t *into_out) const;

    const scoped_ptr_t<serializer_t> inner;
    const scoped_ptr_t<file_account_t> block_writes_io_account;

    // Used to obey the index_write API and make sure we can't possibly make
    // simultaneous racing index_write calls.
    new_mutex_t inner_index_write_mutex;

    // A map of outstanding index write operations, indexed by block id
    std::map<block_id_t, index_write_op_t> outstanding_index_write_ops;

    // Index writes which are currently outstanding keep a pointer to this condition.
    // It is pulsed once the write completes.
    class counted_cond_t : public cond_t,
                           public single_threaded_countable_t<counted_cond_t> {
    };
    counted_t<counted_cond_t> on_inner_index_write_complete;
    bool unhandled_index_write_waiter_exists;

    int num_active_writes;
    int max_active_writes;

    void do_index_write();

    DISABLE_COPYING(merger_serializer_t);
};

#endif /* SERIALIZER_MERGER_HPP_ */
