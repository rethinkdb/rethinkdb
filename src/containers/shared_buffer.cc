// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/shared_buffer.hpp"

#include <stdlib.h>

#include "utils.hpp"

scoped_ptr_t<shared_buf_t> shared_buf_t::create(size_t size) {
    // This allocates size bytes for the data_ field (which is declared as char[1])
    size_t memory_size = sizeof(shared_buf_t) + size - 1;
    void *raw_result = ::rmalloc(memory_size);
    shared_buf_t *result = static_cast<shared_buf_t *>(raw_result);
    result->refcount_ = 0;
#ifndef NDEBUG
    result->debug_size_ = size;
#endif
    return scoped_ptr_t<shared_buf_t>(result);
}
scoped_ptr_t<shared_buf_t> shared_buf_t::create_and_init(size_t size, const char *data) {
    scoped_ptr_t<shared_buf_t> result(shared_buf_t::create(size));
    memcpy(result->data_, data, size);
    return result;
}
void shared_buf_t::operator delete(void *p) {
    ::free(p);
}

char *shared_buf_t::data(size_t offset) {
    // This check is <= instead of < so you can write things like
    //  auto shared_buf = shared_buf_t::create(n);
    //  memcpy(shared_buf->data(0), src, n)
    // and have that still work if n == 0
    rassert(offset <= debug_size_);
    return data_ + offset;
}
const char *shared_buf_t::data(size_t offset) const {
    // See comment above for why this is <= and not <
    rassert(offset <= debug_size_);
    return data_ + offset;
}
