// Copyright 2014 RethinkDB, all rights reserved.
#include "tracking_allocator.hpp"

#include "rdb_protocol/error.hpp"

template <class T, class Allocator>
T* tracking_allocator<T, Allocator>::allocate(size_t n) {
    if (parent->debit(sizeof(T) * n))
        return this->original.allocate(n);
    else
        rfail_toplevel(ql::base_exc_t::GENERIC, "Failed to allocate memory; exceeded allocation threshold");
}

template <class T, class Allocator>
void tracking_allocator<T, Allocator>::deallocate(T* region, size_t n) {
    parent->credit(sizeof(T) * n);
    this->original.deallocate(region, n);
}