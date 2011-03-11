
#ifndef __SERIALIZER_HPP__
#define __SERIALIZER_HPP__

#include "serializer/types.hpp"
#include "server/cmd_args.hpp"

#include "utils2.hpp"
#include "utils.hpp"

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

    /* Some serializer implementations support read-ahead to speed up cache warmup.
    This is supported through a read_ahead_callback_t which gets called whenever the serializer has read-ahead some buf.
    The callee can then decide whether it wants to use the offered buffer of discard it.
    */
    class read_ahead_callback_t {
    public:
        virtual ~read_ahead_callback_t() { }
        /* If the callee returns true, it is responsible to free buf by calling free(buf) in the corresponding serializer. */
        virtual bool offer_read_ahead_buf(ser_block_id_t block_id, void *buf) = 0;
    };
    virtual void register_read_ahead_cb(read_ahead_callback_t *cb) = 0;
    virtual void unregister_read_ahead_cb(read_ahead_callback_t *cb) = 0;
    
    /* Reading a block from the serializer */
    
    struct read_callback_t {
        virtual void on_serializer_read() = 0;
        virtual ~read_callback_t() {}
    };
    virtual bool do_read(ser_block_id_t block_id, void *buf, read_callback_t *callback) = 0;

    virtual ser_transaction_id_t get_current_transaction_id(ser_block_id_t block_id, const void* buf) = 0;
    
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
    struct write_block_callback_t {
        virtual void on_serializer_write_block() = 0;
        virtual ~write_block_callback_t() {}
    };
    struct write_t {
        ser_block_id_t block_id;
        bool recency_specified;
        bool buf_specified;
        repli_timestamp recency;
        const void *buf;   /* If NULL, a deletion */
        bool write_empty_deleted_block;
        write_block_callback_t *callback;
        bool assign_transaction_id;

        friend class log_serializer_t;

        static write_t make(ser_block_id_t block_id_, repli_timestamp recency_, const void *buf_, bool write_empty_deleted_block_, write_block_callback_t *callback_) {
            return write_t(block_id_, true, recency_, true, buf_, write_empty_deleted_block_, callback_, true);
        }

        friend class translator_serializer_t;

    private:
        static write_t make_internal(ser_block_id_t block_id_, const void *buf_, write_block_callback_t *callback_) {
            return write_t(block_id_, false, repli_timestamp::invalid, true, buf_, true, callback_, false);
        }

        write_t(ser_block_id_t block_id_, bool recency_specified_, repli_timestamp recency_,
                bool buf_specified_, const void *buf_, bool write_empty_deleted_block_, write_block_callback_t *callback_, bool assign_transaction_id)
            : block_id(block_id_), recency_specified(recency_specified_), buf_specified(buf_specified_), recency(recency_), buf(buf_), write_empty_deleted_block(write_empty_deleted_block_), callback(callback_), assign_transaction_id(assign_transaction_id) { }
    };
    virtual bool do_write(write_t *writes, int num_writes, write_txn_callback_t *callback) = 0;
    
    /* The size, in bytes, of each serializer block */
    
    virtual block_size_t get_block_size() = 0;
    
    /* max_block_id() and block_in_use() are used by the buffer cache to reconstruct
    the free list of unused block IDs. */
    
    /* Returns a block ID such that every existing block has an ID
    less than that ID. Note that block_in_use(max_block_id() - 1) is
    not guaranteed.  Note that for k > 0, max_block_id() - k might have
    never been created. */
    virtual ser_block_id_t max_block_id() = 0;
    
    /* Checks whether a given block ID exists */
    virtual bool block_in_use(ser_block_id_t id) = 0;

    /* Gets a block's timestamp.  This may return repli_timestamp::invalid. */
    virtual repli_timestamp get_recency(ser_block_id_t id) = 0;

private:
    DISABLE_COPYING(serializer_t);
};

#endif /* __SERIALIZER_HPP__ */
