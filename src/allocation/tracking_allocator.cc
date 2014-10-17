// Copyright 2014 RethinkDB, all rights reserved.
#include "tracking_allocator.hpp"

#include "rdb_protocol/error.hpp"

template <class T, class Allocator>
T* tracking_allocator<T, Allocator>::allocate(size_t n) {
    if (auto p = parent.lock()) {
        if (!(p->debit(sizeof(T) * n)))
            rfail_toplevel(ql::base_exc_t::GENERIC,
                           "Failed to allocate memory; exceeded allocation threshold");
    }
    return this->original.allocate(n);
}

template <class T, class Allocator>
T* tracking_allocator<T, Allocator>::allocate(size_t n, const void *hint) {
    if (auto p = parent.lock()) {
        if (!(p->debit(sizeof(T) * n)))
            rfail_toplevel(ql::base_exc_t::GENERIC,
                           "Failed to allocate memory; exceeded allocation threshold");
    }
    return this->original.allocate(n, hint);
}

template <class T, class Allocator>
void tracking_allocator<T, Allocator>::deallocate(T* region, size_t n) {
    if (auto p = parent.lock()) p->credit(sizeof(T) * n);
    this->original.deallocate(region, n);
}

template <class T, class Allocator>
size_t tracking_allocator<T, Allocator>::max_size() const {
    if (auto p = parent.lock()) return p->left();
    return std::numeric_limits<size_t>::max();
}
