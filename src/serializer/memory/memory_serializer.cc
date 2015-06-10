// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "serializer/memory/memory_serializer.hpp"

#include "concurrency/new_mutex.hpp"
#include "config/args.hpp"

memory_block_token_t::memory_block_token_t(const memory_serializer_t *ser, buf_ptr_t &&d)
    : block_token_t(ser->home_thread()),
      data(std::move(d)) { }

block_size_t memory_block_token_t::block_size() const {
    return data.block_size();
}

buf_ptr_t memory_serializer_t::block_read(const counted_t<block_token_t> &token,
                                          file_account_t *) {
    assert_thread();
    const memory_block_token_t *mem_token
        = static_cast<const memory_block_token_t *>(token.get());
    buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(mem_token->data.block_size());
    memcpy(buf.ser_buffer(),
           mem_token->data.ser_buffer(),
           mem_token->data.block_size().ser_value());
    return buf;
}

block_id_t memory_serializer_t::max_block_id() {
    assert_thread();
    return block_id_t(blocks.size());
}

segmented_vector_t<repli_timestamp_t>
memory_serializer_t::get_all_recencies(block_id_t, block_id_t) {
    assert_thread();
    /* This should only be called before writing any block */
    // TODO: Somehow this is called after some blocks have been created.
    //   That's bad.
    return segmented_vector_t<repli_timestamp_t>();
}

bool memory_serializer_t::get_delete_bit(block_id_t) {
    assert_thread();
    /* This can only be called before writing any block. */
    guarantee(blocks.size() == 0);
    return false;
}

counted_t<block_token_t> memory_serializer_t::index_read(block_id_t block_id) {
    assert_thread();
    guarantee(block_id < blocks.size());
    // TODO: Avoid all this copying
    buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(blocks[block_id].data.block_size());
    memcpy(buf.ser_buffer(),
           blocks[block_id].data.ser_buffer(),
           blocks[block_id].data.block_size().ser_value());
    return counted_t<block_token_t>(new memory_block_token_t(this, std::move(buf)));
}

void memory_serializer_t::index_write(
        new_mutex_in_line_t *mutex_acq,
        const std::vector<index_write_op_t> &write_ops) {
    assert_thread();
    for (const auto &op : write_ops) {
        if (blocks.size() <= op.block_id) {
            blocks.resize_with_zeros(op.block_id + 1);
        }
        if (op.token) {
            if (!op.token->has()) {
                // Deletion
                blocks[op.block_id].data.reset();
            } else {
                const memory_block_token_t *mem_token
                    = static_cast<const memory_block_token_t *>(op.token->get());
                blocks[op.block_id].data
                    = buf_ptr_t::alloc_uninitialized(mem_token->data.block_size());
                memcpy(blocks[op.block_id].data.ser_buffer(),
                       mem_token->data.ser_buffer(),
                       mem_token->data.block_size().ser_value());
            }
        }
        if (op.recency) {
            blocks[op.block_id].recency = op.recency.get();
        }
    }
    mutex_acq->reset();
}

std::vector<counted_t<block_token_t> >
memory_serializer_t::block_writes(
        const std::vector<buf_write_info_t> &write_infos,
        file_account_t *,
        iocallback_t *cb) {
    assert_thread();
    std::vector<counted_t<block_token_t> > result;
    result.reserve(write_infos.size());
    for (const auto &info : write_infos) {
        buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(info.block_size);
        memcpy(buf.ser_buffer(), info.buf, info.block_size.ser_value());
        result.push_back(
            counted_t<block_token_t>(new memory_block_token_t(this, std::move(buf))));
    }
    cb->on_io_complete();
    return result;
}

max_block_size_t memory_serializer_t::max_block_size() const {
    return max_block_size_t::unsafe_make(DEFAULT_BTREE_BLOCK_SIZE);
}
