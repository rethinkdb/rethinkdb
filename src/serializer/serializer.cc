// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/serializer.hpp"

#include "arch/arch.hpp"
#include "boost_utils.hpp"
#include "math.hpp"

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

void debug_print(printf_buffer_t *buf, const buf_write_info_t &info) {
    buf->appendf("bwi{buf=%p, size=%" PRIu32 ", id=%" PRIu64 "}",
                 info.buf, info.block_size.ser_value(), info.block_id);
}
