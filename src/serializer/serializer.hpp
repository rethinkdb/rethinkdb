// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef SERIALIZER_SERIALIZER_HPP_
#define SERIALIZER_SERIALIZER_HPP_

#include <vector>

#include "utils.hpp"
#include <boost/optional.hpp>

#include "arch/types.hpp"
#include "concurrency/cond_var.hpp"
#include "repli_timestamp.hpp"
#include "serializer/types.hpp"

struct index_write_op_t {
    block_id_t block_id;
    // Buf to write. None if not to be modified. Initialized but a null ptr if to be removed from lba.
    boost::optional<intrusive_ptr_t<standard_block_token_t> > token;
    boost::optional<repli_timestamp_t> recency; // Recency, if it should be modified.

    explicit index_write_op_t(block_id_t _block_id,
                              boost::optional<intrusive_ptr_t<standard_block_token_t> > _token = boost::none,
                              boost::optional<repli_timestamp_t> _recency = boost::none)
        : block_id(_block_id), token(_token), recency(_recency) { }
};

/* serializer_t is an abstract interface that describes how each serializer should
behave. It is implemented by log_serializer_t, semantic_checking_serializer_t, and
translator_serializer_t. */

/* Except as otherwise noted, the serializer's methods should only be
   called from the thread it was created on, and it should be
   destroyed on that same thread. */

class serializer_t : public home_thread_mixin_t {
public:
    typedef standard_block_token_t block_token_type;

    serializer_t() { }
    virtual ~serializer_t() { }

    /* The buffers that are used with do_read() and do_write() must be allocated using
    these functions. They can be safely called from any thread. */

    virtual void *malloc() = 0;
    virtual void *clone(void*) = 0; // clones a buf
    virtual void free(void*) = 0;

    /* Allocates a new io account for the underlying file.
    Use delete to free it. */
    file_account_t *make_io_account(int priority);
    virtual file_account_t *make_io_account(int priority, int outstanding_requests_limit) = 0;

    /* Some serializer implementations support read-ahead to speed up cache warmup.
    This is supported through a serializer_read_ahead_callback_t which gets called whenever the serializer has read-ahead some buf.
    The callee can then decide whether it wants to use the offered buffer of discard it.
    */
    virtual void register_read_ahead_cb(serializer_read_ahead_callback_t *cb) = 0;
    virtual void unregister_read_ahead_cb(serializer_read_ahead_callback_t *cb) = 0;

    /* Reading a block from the serializer */
    // Non-blocking variant
    virtual void block_read(const intrusive_ptr_t<standard_block_token_t>& token, void *buf, file_account_t *io_account, iocallback_t *cb) = 0;

    // Blocking variant (requires coroutine context). Has default implementation.
    virtual void block_read(const intrusive_ptr_t<standard_block_token_t>& token, void *buf, file_account_t *io_account) = 0;

    /* The index stores three pieces of information for each ID:
     * 1. A pointer to a data block on disk (which may be NULL)
     * 2. A repli_timestamp_t, called the "recency"
     * 3. A boolean, called the "delete bit" */

    /* max_block_id() and get_delete_bit() are used by the buffer cache to reconstruct
    the free list of unused block IDs. */

    /* Returns a block ID such that every existing block has an ID less than
     * that ID. Note that index_read(max_block_id() - 1) is not guaranteed to be
     * non-NULL. Note that for k > 0, max_block_id() - k might have never been
     * created. */
    virtual block_id_t max_block_id() = 0;

    /* Gets a block's timestamp.  This may return repli_timestamp_t::invalid. */
    virtual repli_timestamp_t get_recency(block_id_t id) = 0;

    /* Reads the block's delete bit. */
    virtual bool get_delete_bit(block_id_t id) = 0;

    /* Reads the block's actual data */
    virtual intrusive_ptr_t<standard_block_token_t> index_read(block_id_t block_id) = 0;

    /* index_write() applies all given index operations in an atomic way */
    virtual void index_write(serializer_transaction_t *ser_txn,
                             const std::vector<index_write_op_t>& write_ops, file_account_t *io_account) = 0;

    /* Non-blocking variants */
    virtual intrusive_ptr_t<standard_block_token_t>
    block_write(serializer_transaction_t *ser_txn,
                const void *buf, block_id_t block_id, file_account_t *io_account, iocallback_t *cb) = 0;
    virtual intrusive_ptr_t<standard_block_token_t>
    block_write(serializer_transaction_t *ser_txn,
                const void *buf, block_id_t block_id, file_account_t *io_account) = 0;

    virtual block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf) const = 0;

    /* The size, in bytes, of each serializer block */
    virtual block_size_t get_block_size() = 0;

    /* Return true if no other processes have the file locked */
    // TODO: Fix this bad API.  It should not be possible to even construct a serializer if somebody else has the file lock.
    virtual bool coop_lock_and_check() = 0;

private:
    DISABLE_COPYING(serializer_t);
};


// The do_write interface is now obvious helper functions

struct serializer_write_launched_callback_t {
    virtual void on_write_launched(const intrusive_ptr_t<standard_block_token_t>& token) = 0;
    virtual ~serializer_write_launched_callback_t() {}
};
struct serializer_write_t {
    block_id_t block_id;

    enum { UPDATE, DELETE, TOUCH } action_type;
    union {
        struct {
            const void *buf;
            repli_timestamp_t recency;
            iocallback_t *io_callback;
            serializer_write_launched_callback_t *launch_callback;
        } update;
        struct {
            repli_timestamp_t recency;
        } touch;
    } action;

    static serializer_write_t make_touch(block_id_t block_id, repli_timestamp_t recency);
    static serializer_write_t make_update(block_id_t block_id, repli_timestamp_t recency, const void *buf,
                                          iocallback_t *io_callback = NULL,
                                          serializer_write_launched_callback_t *launch_callback = NULL);
    static serializer_write_t make_delete(block_id_t block_id);
};

/* A bad wrapper for doing block writes and index writes.
 */
void do_writes(serializer_t *ser, const std::vector<serializer_write_t>& writes, file_account_t *io_account);


// Helpers for default implementations that can be used on log_serializer_t.

template <class serializer_type>
void serializer_index_write(serializer_type *ser, serializer_transaction_t *ser_txn,
                            const index_write_op_t& op, file_account_t *io_account) {
    std::vector<index_write_op_t> ops;
    ops.push_back(op);
    return ser->index_write(ser_txn, ops, io_account);
}

#endif /* SERIALIZER_SERIALIZER_HPP_ */
