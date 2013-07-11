// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "serializer/serializer.hpp"
#include "arch/arch.hpp"

file_account_t *serializer_t::make_io_account(int priority) {
    assert_thread();
    return make_io_account(priority, UNLIMITED_OUTSTANDING_REQUESTS);
}

serializer_write_t serializer_write_t::make_touch(block_id_t block_id, repli_timestamp_t recency) {
    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = TOUCH;
    w.action.touch.recency = recency;
    return w;
}

serializer_write_t serializer_write_t::make_update(
        block_id_t block_id, repli_timestamp_t recency, const void *buf,
        iocallback_t *io_callback, serializer_write_launched_callback_t *launch_callback) {
    guarantee(io_callback != NULL);
    guarantee(launch_callback != NULL);

    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = UPDATE;
    w.action.update.buf = buf;
    w.action.update.recency = recency;
    w.action.update.io_callback = io_callback;
    w.action.update.launch_callback = launch_callback;
    return w;
}

serializer_write_t serializer_write_t::make_delete(block_id_t block_id) {
    serializer_write_t w;
    w.block_id = block_id;
    w.action_type = DELETE;
    return w;
}

ser_buffer_t *convert_buffer_cache_buf_to_ser_buffer(const void *buf) {
    return static_cast<ser_buffer_t *>(const_cast<void *>(buf)) - 1;
}

void do_writes(serializer_t *ser, const std::vector<serializer_write_t> &writes, file_account_t *io_account) {
    ser->assert_thread();
    std::vector<index_write_op_t> index_write_ops;
    index_write_ops.reserve(writes.size());

    // Step 1: Write buffers to disk and assemble index operations
    std::vector<buf_write_info_t> write_infos;
    write_infos.reserve(writes.size());

    struct : public iocallback_t, public cond_t {
        void on_io_complete() {
            pulse();
        }
    } block_write_cond;

    for (size_t i = 0; i < writes.size(); ++i) {
        if (writes[i].action_type == serializer_write_t::UPDATE) {
            write_infos.push_back(buf_write_info_t(convert_buffer_cache_buf_to_ser_buffer(writes[i].action.update.buf),
                                                   ser->get_block_size(),  // RSI: Use actual block size.
                                                   writes[i].block_id));
        }
    }

    std::vector<counted_t<standard_block_token_t> > tokens;
    if (!write_infos.empty()) {
        tokens = ser->block_writes(write_infos, io_account, &block_write_cond);
    } else {
        block_write_cond.pulse();
    }
    guarantee(tokens.size() == write_infos.size());

    size_t tokens_index = 0;
    for (size_t i = 0; i < writes.size(); ++i) {
        switch (writes[i].action_type) {
        case serializer_write_t::UPDATE: {
            guarantee(tokens_index < tokens.size());
            index_write_ops.push_back(index_write_op_t(writes[i].block_id,
                                                       tokens[tokens_index],
                                                       writes[i].action.update.recency));

            if (writes[i].action.update.launch_callback != NULL) {
                writes[i].action.update.launch_callback->on_write_launched(tokens[tokens_index]);
            }

            ++tokens_index;
        } break;
        case serializer_write_t::DELETE: {
            index_write_ops.push_back(index_write_op_t(writes[i].block_id,
                                                       counted_t<standard_block_token_t>(),
                                                       repli_timestamp_t::invalid));
        } break;
        case serializer_write_t::TOUCH: {
            index_write_ops.push_back(index_write_op_t(writes[i].block_id,
                                                       boost::none,
                                                       writes[i].action.touch.recency));
        } break;
        default:
            unreachable();
        }
    }

    guarantee(tokens_index == tokens.size());
    guarantee(index_write_ops.size() == writes.size());

    // Step 2: Wait on all writes to finish
    block_write_cond.wait();

    // Step 2.5: Call these annoying io_callbacks.
    for (size_t i = 0; i < writes.size(); ++i) {
        if (writes[i].action_type == serializer_write_t::UPDATE) {
            writes[i].action.update.io_callback->on_io_complete();
        }
    }

    // Step 3: Commit the transaction to the serializer
    ser->index_write(index_write_ops, io_account);
}

void serializer_data_ptr_t::free() {
    rassert(ptr_.has());
    ptr_.reset();
}

void serializer_data_ptr_t::init_malloc(serializer_t *ser) {
    rassert(!ptr_.has());
    ptr_ = ser->malloc();
}

void serializer_data_ptr_t::init_clone(serializer_t *ser, const serializer_data_ptr_t &other) {
    rassert(other.ptr_.has());
    rassert(!ptr_.has());
    ptr_ = ser->clone(other.ptr_.get());
}

counted_t<standard_block_token_t> serializer_block_write(serializer_t *ser, ser_buffer_t *buf,
                                                         block_size_t block_size,
                                                         block_id_t block_id, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;
    ser_buffer_t *ser_buf = convert_buffer_cache_buf_to_ser_buffer(buf);

    std::vector<counted_t<standard_block_token_t> > tokens
        = ser->block_writes({ buf_write_info_t(ser_buf, block_size, block_id) },
                            io_account, &cb);
    guarantee(tokens.size() == 1);
    cb.wait();
    return tokens[0];

}

