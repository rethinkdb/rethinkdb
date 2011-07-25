#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

#include "arch/arch.hpp"
#include "serializer/types.hpp"

#include <boost/optional.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/variant/variant.hpp>
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
        virtual bool offer_read_ahead_buf(block_id_t block_id, void *buf, repli_timestamp_t recency_timestamp) = 0;
    };
    virtual void register_read_ahead_cb(read_ahead_callback_t *cb) = 0;
    virtual void unregister_read_ahead_cb(read_ahead_callback_t *cb) = 0;

    class block_token_t {
    public:
        virtual ~block_token_t() { }
    };

    /* Reading a block from the serializer */
    // Non-blocking variant
    virtual void block_read(boost::shared_ptr<block_token_t> token, void *buf, file_t::account_t *io_account, iocallback_t *cb) = 0;

    // Blocking variant (requires coroutine context). Has default implementation.
    virtual void block_read(boost::shared_ptr<block_token_t> token, void *buf, file_t::account_t *io_account);

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
    virtual boost::shared_ptr<block_token_t> index_read(block_id_t block_id) = 0;

    struct index_write_op_t {
        block_id_t block_id;
        // Buf to write. None if not to be modified. Initialized but a null ptr if to be removed from lba.
        boost::optional<boost::shared_ptr<block_token_t> > token;
        boost::optional<repli_timestamp_t> recency; // Recency, if it should be modified.
        boost::optional<bool> delete_bit;           // Delete bit, if it should be modified.

        index_write_op_t(block_id_t block_id,
                         boost::optional<boost::shared_ptr<block_token_t> > token = boost::none,
                         boost::optional<repli_timestamp_t> recency = boost::none,
                         boost::optional<bool> delete_bit = boost::none);
    };

    /* index_write() applies all given index operations in an atomic way */
    virtual void index_write(const std::vector<index_write_op_t>& write_ops, file_t::account_t *io_account) = 0;
    // convenience wrapper for a single index_write_op_t
    void index_write(const index_write_op_t &op, file_t::account_t *io_account);

    /* Non-blocking variants */
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account, iocallback_t *cb) = 0;
    // `block_write(buf, acct, cb)` must behave identically to `block_write(buf, NULL_BLOCK_ID, acct, cb)`
    // a default implementation is provided using this
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account, iocallback_t *cb);

    /* Blocking variants (use in coroutine context) with and without known block_id */
    // these have default implementations in serializer.cc in terms of the non-blocking variants above
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, file_t::account_t *io_account);
    virtual boost::shared_ptr<block_token_t> block_write(const void *buf, block_id_t block_id, file_t::account_t *io_account);

    virtual block_sequence_id_t get_block_sequence_id(block_id_t block_id, const void* buf) = 0;

    // New do_write interface
    struct write_launched_callback_t {
        virtual void on_write_launched(boost::shared_ptr<block_token_t> token) = 0;
        virtual ~write_launched_callback_t() {}
    };
    struct write_t {
        block_id_t block_id;
        struct update_t {
            const void *buf;
            repli_timestamp_t recency;
            iocallback_t *io_callback;
            write_launched_callback_t *launch_callback;
        };
        struct delete_t { bool write_zero_block; };
        struct touch_t { repli_timestamp_t recency; };
        // if none, indicates just a recency update.
        typedef boost::variant<update_t, delete_t, touch_t> action_t;
        action_t action;

        static write_t make_touch(block_id_t block_id, repli_timestamp_t recency);
        static write_t make_update(block_id_t block_id, repli_timestamp_t recency, const void *buf,
                                   iocallback_t *io_callback = NULL,
                                   write_launched_callback_t *launch_callback = NULL);
        static write_t make_delete(block_id_t block_id, bool write_zero_block = true);
        write_t(block_id_t block_id, action_t action);
    };

    /* Performs a group of writes. Must be called from coroutine context. cb is called (in coroutine
     * context) as soon as all block writes have been sent to the serializer (but not necessarily
     * completed). Returns when all writes are finished and the LBA has been updated.
     *
     * Note that this is not virtual. It is implement in terms of block_write() and index_write(),
     * and not meant to be overridden in subclasses.
     */
    void do_write(std::vector<write_t> writes, file_t::account_t *io_account);

    /* The size, in bytes, of each serializer block */

    virtual block_size_t get_block_size() = 0;

private:
    DISABLE_COPYING(serializer_t);
};

#endif /* __SERIALIZER_HPP__ */
