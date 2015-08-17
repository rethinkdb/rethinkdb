// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/shared_buffer.hpp"

#include <stdlib.h>

#include "utils.hpp"

counted_t<shared_buf_t> shared_buf_t::create(size_t size) {
    // This allocates size bytes for the data_ field (which is declared as char[1])
    size_t memory_size = sizeof(shared_buf_t) + size - 1;
    void *raw_result = ::rmalloc(memory_size);
    shared_buf_t *result = static_cast<shared_buf_t *>(raw_result);
    result->refcount_ = 0;
    result->size_ = size;
    return counted_t<shared_buf_t>(result);
}

void shared_buf_t::operator delete(void *p) {
    ::free(p);
}

char *shared_buf_t::data(size_t offset) {
    return data_ + offset;
}

const char *shared_buf_t::data(size_t offset) const {
    return data_ + offset;
}

size_t shared_buf_t::size() const {
    return size_;
}
