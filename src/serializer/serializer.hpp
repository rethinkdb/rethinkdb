
#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

#include "serializer/types.hpp"
#include "server/cmd_args.hpp"

#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>
#include "utils2.hpp"
#include "utils.hpp"
#include "concurrency/cond_var.hpp"

/* serializer_t is an abstract interface that describes how each serializer should
behave. It is implemented by log_serializer_t, semantic_checking_serializer_t, and
others. */

struct serializer_t :
    /* Except as otherwise noted, the serializer's methods should only be called from the
    thread it was created on, and it should be destroyed on that same thread. */
    public home_thread_mixin_t
{
    serializer_t() { }
    virtual ~serializer_t() {}

    /* The buffers that are used with do_read() and do_write() must be allocated using
    these functions. They can be safely called from any thread. */

    virtual void *malloc() = 0;
    virtual void *clone(void*) = 0; // clones a buf
    virtual void free(void*) = 0;

    /* Allocates a new io account for the underlying file.
    Use delete to free it. */
    virtual file_t::account_t *make_io_account(int priority, int outstanding_requests_limit = UNLIMITED_OUTSTANDING_REQUESTS) = 0;

    /* Some serializer implementations support read-ahead to speed up cache warmup.
    This is supported through a read_ahead_callback_t which gets called whenever the serializer has read-ahead some buf.
    The callee can then decide whether it wants to use the offered buffer of discard it.
    */
    class read_ahead_callback_t {
    public:
        virtual ~read_ahead_callback_t() { }
        /* If the callee returns true, it is responsible to free buf by calling free(buf) in the corresponding serializer. */
        virtual bool offer_read_ahead_buf(block_id_t block_id, void *buf, repli_timestamp recency_timestamp) = 0;
    };
    virtual void register_read_ahead_cb(read_ahead_callback_t *cb) = 0;
    virtual void unregister_read_ahead_cb(read_ahead_callback_t *cb) = 0;

    class block_token_t {
    public:
        virtual ~block_token_t() { }
    };

    /* Reading a block from the serializer */

    /* Require coroutine context, block until data is available */
    virtual void block_read(boost::shared_ptr<block_token_t> token, void *buf, file_t::account_t *io_account) = 0;

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

    /* Gets a block's timestamp.  This may return repli_timestamp::invalid. */
    virtual repli_timestamp get_recency(block_id_t id) = 0;

    /* Reads the block's delete bit. */
    virtual bool get_delete_bit(block_id_t id) = 0;

    /* Reads the block's actual data */
    virtual boost::shared_ptr<block_token_t> index_read(block_id_t block_id) = 0;

    struct index_write_op_t {
        block_id_t block_id;
        // Buf to write. None if not to be modified. Initialized but a null ptr if to be removed from lba.
        boost::optional<boost::shared_ptr<block_token_t> > token;
        boost::optional<bool> delete_bit;         // Delete bit, if it should be modified.
        boost::optional<repli_timestamp> recency; // Recency, if it should be modified.

        index_write_op_t(block_id_t block_id) : block_id(block_id) {}
        index_write_op_t(block_id_t block_id, boost::shared_ptr<block_token_t> token)
            : block_id(block_id), token(token) {}
        index_write_op_t(block_id_t block_id, boost::optional<boost::shared_ptr<block_token_t> > token,
                         boost::optional<bool> delete_bit, boost::optional<repli_timestamp> recency)
            : block_id(block_id), token(token), delete_bit(delete_bit), recency(recency) {}
    };

    /* index_write() applies all given index operations in an atomic way */
    virtual void index_write(const std::vector<index_write_op_t>& write_ops, file_t::account_t *io_account) = 0;

    /* Non-blocking variant */
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account, iocallback_t *cb) = 0;
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account, iocallback_t *cb) = 0;

    /* Blocking variant (use in coroutine context) with and without known block_id */
    // these have default implementations in serializer.cc in terms of the non-blocking variants above
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account);
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account);

    virtual ser_block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf) = 0;

    /* TODO: The following part is all just wrapper code. It should be removed eventually */

    /* DEPRECATED wrapper code begins here! */

    // TODO: Remove this legacy interface at some point
    struct read_callback_t {
        virtual void on_serializer_read() = 0;
        virtual ~read_callback_t() {}
    };

    /*
    do_read() is DEPRECATED.
    Please use block_read(index_read(...), ...) to get the same functionality
    in a coroutine aware manner
    */
    bool do_read(block_id_t block_id, void *buf, file_t::account_t *io_account, read_callback_t *callback);

    /* do_write() updates or deletes a group of bufs.

    Each write_t passed to do_write() identifies an update or deletion. If 'buf' is NULL, then it
    represents a deletion. If 'buf' is non-NULL, then it identifies an update, and the given
    callback will be called as soon as the data has been copied out of 'buf'. If the entire
    transaction completes immediately, it will return 'true'; otherwise, it will return 'false' and
    call the given callback at a later date.

    'writes' can be freed as soon as do_write() returns. */

    struct write_txn_callback_t {
        virtual void on_serializer_write_txn() = 0;
        virtual ~write_txn_callback_t() {}
    };
    struct write_tid_callback_t {
        virtual void on_serializer_write_tid() = 0;
        virtual ~write_tid_callback_t() {}
    };
    struct write_block_callback_t {
        virtual void on_serializer_write_block() = 0;
        virtual ~write_block_callback_t() {}
    };
    struct write_t {
        block_id_t block_id;
        bool recency_specified;
        bool buf_specified;
        repli_timestamp recency;
        const void *buf;   /* If NULL, a deletion */
        bool write_empty_deleted_block;
        write_block_callback_t *callback;

        friend class log_serializer_t;

        static write_t make_touch(block_id_t block_id_, repli_timestamp recency_, write_block_callback_t *callback_) {
            return write_t(block_id_, true, recency_, false, NULL, true, callback_);
        }

        static write_t make(block_id_t block_id_, repli_timestamp recency_, const void *buf_, bool write_empty_deleted_block_, write_block_callback_t *callback_) {
            return write_t(block_id_, true, recency_, true, buf_, write_empty_deleted_block_, callback_);
        }

    private:
        write_t(block_id_t block_id_, bool recency_specified_, repli_timestamp recency_, bool buf_specified_,
                const void *buf_, bool write_empty_deleted_block_, write_block_callback_t *callback_)
            : block_id(block_id_), recency_specified(recency_specified_), buf_specified(buf_specified_)
            , recency(recency_), buf(buf_), write_empty_deleted_block(write_empty_deleted_block_)
            , callback(callback_) { }
    };

    /* tid_callback is called as soon as new transaction ids have been assigned to each written block,
     * callback gets called when all data has been written to disk */
    /* do_write() is DEPRECATED.
     * Please use block_write and index_write instead */
    bool do_write(write_t *writes, int num_writes, file_t::account_t *io_account,
                  write_txn_callback_t *callback, write_tid_callback_t *tid_callback = NULL);
    /* DEPRECATED wrapper code ends here! */

    /* The size, in bytes, of each serializer block */

    virtual block_size_t get_block_size() = 0;

private:
    DISABLE_COPYING(serializer_t);
};

#endif /* __SERIALIZER_HPP__ */
