// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/serializer.hpp"

#include "arch/arch.hpp"
#include "boost_utils.hpp"

void debug_print(printf_buffer_t *buf, const index_write_op_t &write_op) {
    buf->appendf("iwop{id=%" PRIu64 ", token=", write_op.block_id);
    debug_print(buf, write_op.token);
    buf->appendf(", recency=");
    debug_print(buf, write_op.recency);
    buf->appendf("}");
}

file_account_t *serializer_t::make_io_account(int priority) {
    assert_thread();
    return make_io_account(priority, UNLIMITED_OUTSTANDING_REQUESTS);
}

ser_buffer_t *convert_buffer_cache_buf_to_ser_buffer(const void *buf) {
    return static_cast<ser_buffer_t *>(const_cast<void *>(buf)) - 1;
}

counted_t<standard_block_token_t> serializer_block_write(serializer_t *ser, ser_buffer_t *buf,
                                                         block_size_t block_size,
                                                         block_id_t block_id, file_account_t *io_account) {
    struct : public cond_t, public iocallback_t {
        void on_io_complete() { pulse(); }
    } cb;

    std::vector<counted_t<standard_block_token_t> > tokens
        = ser->block_writes({ buf_write_info_t(buf, block_size, block_id) },
                            io_account, &cb);
    guarantee(tokens.size() == 1);
    cb.wait();
    return tokens[0];

}

void debug_print(printf_buffer_t *buf, const buf_write_info_t &info) {
    buf->appendf("bwi{buf=%p, size=%" PRIu32 ", id=%" PRIu64 "}",
                 info.buf, info.block_size.ser_value(), info.block_id);
}
