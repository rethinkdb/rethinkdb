// Copyright 2010-2012 RethinkDB, all rights reserved.
#define __STDC_LIMIT_MACROS

#include "containers/data_buffer.hpp"

#include <stdint.h>

#include "utils.hpp"


void debug_print(printf_buffer_t *buf, const counted_t<data_buffer_t>& ptr) {
    if (!ptr.has()) {
        buf->appendf("databuf_ptr{null}");
    } else {
        buf->appendf("databuf_ptr{");
        const char *bytes = ptr->buf();
        debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(bytes), ptr->size());
        buf->appendf("}");
    }
}

counted_t<data_buffer_t> data_buffer_t::create(int64_t size) {
    static_assert(sizeof(data_buffer_t) == sizeof(ref_count_) + sizeof(size_),
                  "data_buffer_t is not a packed struct type");

    rassert(size >= 0 && static_cast<uint64_t>(size) <= SIZE_MAX - sizeof(data_buffer_t));
    data_buffer_t *b = static_cast<data_buffer_t *>(malloc(sizeof(data_buffer_t) + size));
    b->ref_count_ = 0;
    b->size_ = size;
    return counted_t<data_buffer_t>(b);
}
