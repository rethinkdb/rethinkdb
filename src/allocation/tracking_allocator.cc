// Copyright 2014 RethinkDB, all rights reserved.
#include "tracking_allocator.hpp"

#include "rdb_protocol/error.hpp"

template <class T>
T *tracking_allocator_t<T>::allocate(size_t n) {
    if (auto p = parent.lock()) {
        if (!(p->debit(sizeof(T) * n))) {
            rfail_toplevel(ql::base_exc_t::GENERIC,
                           "Failed to allocate memory; exceeded allocation threshold");
        }
    }
    return std::allocator<T>().allocate(n);
}

template <class T>
T *tracking_allocator_t<T>::allocate(size_t n, const void *hint) {
    if (auto p = parent.lock()) {
        if (!(p->debit(sizeof(T) * n))) {
            rfail_toplevel(ql::base_exc_t::GENERIC,
                           "Failed to allocate memory; exceeded allocation threshold");
        }
    }
    return std::allocator<T>().allocate(n, hint);
}

template <class T>
void tracking_allocator_t<T>::deallocate(T *region, size_t n) {
    if (auto p = parent.lock()) p->credit(sizeof(T) * n);
    return std::allocator<T>().deallocate(region, n);
}

template <class T>
size_t tracking_allocator_t<T>::max_size() const {
    if (auto p = parent.lock()) return p->left();
    return std::numeric_limits<size_t>::max();
}
