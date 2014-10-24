// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "containers/shared_buffer.hpp"

#include <memory>

#include "allocation/tracking_allocator.hpp"
#include "allocation/unusual_allocator.hpp"
#include "utils.hpp"
#include "rdb_protocol/error.hpp"

counted_t<shared_buf_t> shared_buf_t::create(size_t size) {
    return counted_t<shared_buf_t>(new(size) shared_buf_t(size));
}

counted_t<shared_buf_t> shared_buf_t::create(size_t size,
                                             std::shared_ptr<tracking_allocator_factory_t> f) {
    unusual_size_allocator_t<shared_buf_t> a(f, size);
    shared_buf_t *result = make<shared_buf_t>(std::allocator_arg, a, size);
    counted_set_deleter(result,
                        std::unique_ptr<deallocator_alloc_t<unusual_size_allocator_t<shared_buf_t> > >
                        (new deallocator_alloc_t<unusual_size_allocator_t<shared_buf_t> >(a, result)));
    return counted_t<shared_buf_t>(result);
}

void *shared_buf_t::operator new(size_t size, size_t bytes) {
    return ::rmalloc(size + bytes);
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
