// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/data_buffer.hpp"

#include <stdint.h>

#include "debug.hpp"
#include "utils.hpp"
#include "allocation/unusual_allocator.hpp"

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
    rassert(size >= 0);
    return counted_t<data_buffer_t>(new(size) data_buffer_t(size));
}

counted_t<data_buffer_t> data_buffer_t::create(size_t size,
                                               std::shared_ptr<tracking_allocator_factory_t> f) {
    unusual_size_allocator_t<data_buffer_t> a(f, size);
    data_buffer_t *result = make<data_buffer_t>(std::allocator_arg, a, size);
    counted_set_deleter(result,
                        std::unique_ptr<deallocator_alloc_t<unusual_size_allocator_t<data_buffer_t> > >
                        (new deallocator_alloc_t<unusual_size_allocator_t<data_buffer_t> >(a, result)));
    return counted_t<data_buffer_t>(result);
}

void *data_buffer_t::operator new(size_t size, size_t bytes) {
    static_assert(sizeof(data_buffer_t) == sizeof(ref_count_) + sizeof(size_) + sizeof(deleter_),
                  "data_buffer_t is not a packed struct type");

    rassert(size == sizeof(data_buffer_t));
    rassert(static_cast<uint64_t>(size) <= SIZE_MAX - sizeof(data_buffer_t));
    return ::rmalloc(size + bytes);
}

void data_buffer_t::operator delete(void *ptr) noexcept {
    ::free(ptr);
}
