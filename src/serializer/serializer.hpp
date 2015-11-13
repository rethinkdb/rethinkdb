// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef SERIALIZER_SERIALIZER_HPP_
#define SERIALIZER_SERIALIZER_HPP_

#include <vector>

#include "utils.hpp"
#include <boost/optional.hpp>

#include "arch/types.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/segmented_vector.hpp"
#include "repli_timestamp.hpp"
#include "serializer/types.hpp"

class buf_ptr_t;
class new_mutex_in_line_t;

struct index_write_op_t {
    block_id_t block_id;
    // Buf to write.  boost::none if not to be modified.  Initialized to an empty
    // counted_t if the block is to be deleted.
    boost::optional<counted_t<standard_block_token_t> > token;
    // Recency, if it should be modified.  (It's unmodified when the data block
    // manager moves blocks around while garbage collecting.)
    boost::optional<repli_timestamp_t> recency;

    explicit index_write_op_t(block_id_t _block_id,
                              boost::optional<counted_t<standard_block_token_t> > _token = boost::none,
                              boost::optional<repli_timestamp_t> _recency = boost::none)
        : block_id(_block_id), token(_token), recency(_recency) { }
};

void debug_print(printf_buffer_t *buf, const index_write_op_t &write_op);

/* serializer_t is an abstract interface that describes how each serializer should
behave. It is implemented by merger_serializer_t, log_serializer_t, and
translator_serializer_t. */

/* Except as otherwise noted, the serializer's methods should only be
   called from the thread it was created on, and it should be
   destroyed on that same thread. */

class serializer_t : public home_thread_mixin_t {
public:
    typedef standard_block_token_t block_token_type;

    serializer_t() { }
    virtual ~serializer_t() { }

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

    // Reading a block from the serializer.  Reads a block, blocks the coroutine.
    virtual buf_ptr_t block_read(const counted_t<standard_block_token_t> &token,
                               file_account_t *io_account) = 0;

    /* The index stores three pieces of information for each ID:
     * 1. A pointer to a data block on disk (which may be NULL)
     * 2. A repli_timestamp_t, called the "recency"
     * 3. A boolean, called the "delete bit" */

    /* end_block_id() / end_aux_block_id() and get_delete_bit() are used by the
    buffer cache to reconstruct the free list of unused block IDs. */

    /* Returns a block ID such that every existing regular/aux block has an
     * ID less than that ID. Note that index_read(end_block_id() - 1) is not
     * guaranteed to be non-NULL. Note that for k > 0, end_block_id() - k might
     * have never been created. */
    virtual block_id_t end_block_id() = 0;
    virtual block_id_t end_aux_block_id() = 0;

    /* Returns all recencies, for all block ids of the form first + step * k, for k =
       0, 1, 2, 3, ..., in order by block id.  Non-existant block ids have recency
       repli_timestamp_t::invalid.  You must only call this before _writing_ a
       block to this serializer_t instance, because otherwise the information you get
       back could be wrong. */
    virtual segmented_vector_t<repli_timestamp_t>
    get_all_recencies(block_id_t first, block_id_t step) = 0;

    /* Returns all recencies, indexed by block id.  (See above.) */
    segmented_vector_t<repli_timestamp_t> get_all_recencies() {
        return get_all_recencies(0, 1);
    }

    /* Reads the block's delete bit.  You must only call this on startup, before
       _writing_ a block to this serializer_t instance, because otherwise the
       information you get back could be wrong. */
    virtual bool get_delete_bit(block_id_t id) = 0;

    /* Reads the block's actual data */
    virtual counted_t<standard_block_token_t> index_read(block_id_t block_id) = 0;

    // Applies all given index operations in an atomic way.  The mutex_acq is for a
    // mutex belonging to the _caller_, used by the caller for pipelining, for
    // ensuring that different index write operations do not cross each other.
    // Once `on_writes_reflected` is called, the serializer guarantees that any
    // subsequent call to `index_read` is going to see the index changes, even
    // though they might not have been persisted to disk yet.
    virtual void index_write(new_mutex_in_line_t *mutex_acq,
                             const std::function<void()> &on_writes_reflected,
                             const std::vector<index_write_op_t> &write_ops) = 0;

    // Returns block tokens in the same order as write_infos.
    virtual std::vector<counted_t<standard_block_token_t> >
    block_writes(const std::vector<buf_write_info_t> &write_infos,
                 file_account_t *io_account,
                 iocallback_t *cb) = 0;

    /* The maximum size (and right now the typical size) that a block can have. */
    virtual max_block_size_t max_block_size() const = 0;

    /* Return true if no other processes have the file locked */
    virtual bool coop_lock_and_check() = 0;

    /* Return true if the garbage collector is active */
    virtual bool is_gc_active() const = 0;

private:
    DISABLE_COPYING(serializer_t);
};


#endif /* SERIALIZER_SERIALIZER_HPP_ */
