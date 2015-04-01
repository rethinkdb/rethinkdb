// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef SERIALIZER_MEMORY_MEMORY_SERIALIZER_HPP_
#define SERIALIZER_MEMORY_MEMORY_SERIALIZER_HPP_

#include <map>
#include <string>
#include <vector>
#include <list>

#include "containers/counted.hpp"
#include "serializer/buf_ptr.hpp"
#include "serializer/serializer.hpp"

class memory_serializer_t;

struct memory_block_token_t : public block_token_t {
public:
    block_size_t block_size() const;

private:
    friend class memory_serializer_t;

    memory_block_token_t(const memory_serializer_t *ser, buf_ptr_t &&d);

    void do_destroy() { }

    buf_ptr_t data;
};

class memory_serializer_t :
    public serializer_t {
public:
    memory_serializer_t() { }

    /* The memory serializer does not support io accounts, because there's no io.
    We return a dummy file_account_t though to not confuse the caller. */
    file_account_t *make_io_account(int, int) {
        return new file_account_t(nullptr, 0);
    }

    /* Not supported/needed for the memory serializer... */
    void register_read_ahead_cb(serializer_read_ahead_callback_t *) { }
    void unregister_read_ahead_cb(serializer_read_ahead_callback_t *) { }

    /* Reading a block from the serializer.  Reads a block, blocks the coroutine. */
    buf_ptr_t block_read(const counted_t<block_token_t> &token,
                         file_account_t *io_account);

    /* Returns a block ID such that every existing block has an ID less than
     * that ID. Note that index_read(max_block_id() - 1) is not guaranteed to be
     * non-NULL. Note that for k > 0, max_block_id() - k might have never been
     * created. */
    block_id_t max_block_id();

    /* Can only be called before writing any block. */
    segmented_vector_t<repli_timestamp_t>
    get_all_recencies(block_id_t first, block_id_t step);

    /* This can only be called on startup, before writing any block. */
    bool get_delete_bit(block_id_t id);

    /* Reads the block's index data */
    counted_t<block_token_t> index_read(block_id_t block_id);

    /* Applies all given index operations in an atomic way. */
    void index_write(new_mutex_in_line_t *mutex_acq,
                     const std::vector<index_write_op_t> &write_ops);

    /* Returns block tokens in the same order as write_infos. */
    std::vector<counted_t<block_token_t> >
    block_writes(const std::vector<buf_write_info_t> &write_infos,
                 file_account_t *io_account,
                 iocallback_t *cb);

    /* The maximum size (and right now the typical size) that a block can have. */
    max_block_size_t max_block_size() const;

    bool coop_lock_and_check() { return true; }
    bool is_gc_active() const { return false; }

private:
    struct index_block_t {
        index_block_t() : recency(repli_timestamp_t::invalid) { }
        buf_ptr_t data;
        repli_timestamp_t recency;
    };
    segmented_vector_t<index_block_t> blocks;

    DISABLE_COPYING(memory_serializer_t);
};

#endif /* SERIALIZER_MEMORY_MEMORY_SERIALIZER_HPP_ */
