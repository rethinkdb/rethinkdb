// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "serializer/serializer.hpp"

#include "arch/arch.hpp"
#include "arch/runtime/runtime.hpp"
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

void counted_add_ref(block_token_t *p) {
    DEBUG_VAR intptr_t res = __sync_add_and_fetch(&p->ref_count_, 1);
    rassert(res > 0);
}

void counted_release(block_token_t *p) {
    struct destroyer_t : public linux_thread_message_t {
        void on_thread_switch() {
            rassert(p->ref_count_ == 0);
            p->do_destroy();
            delete this;
        }
        block_token_t *p;
    };

    intptr_t res = __sync_sub_and_fetch(&p->ref_count_, 1);
    rassert(res >= 0);
    if (res == 0) {
        if (get_thread_id() == p->serializer_thread_) {
            p->do_destroy();
        } else {
            destroyer_t *destroyer = new destroyer_t;
            destroyer->p = p;
            DEBUG_VAR bool cont = continue_on_thread(p->serializer_thread_,
                                                     destroyer);
            rassert(!cont);
        }
    }
}

void debug_print(printf_buffer_t *buf,
                 const counted_t<block_token_t> &token) {
    if (token.has()) {
        buf->appendf("block_token{+%" PRIu32 "}",
                     token->block_size().ser_value());
    } else {
        buf->appendf("nil");
    }
}

