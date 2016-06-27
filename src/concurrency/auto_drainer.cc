// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "concurrency/auto_drainer.hpp"
#include "concurrency/interruptor.hpp"

#include "arch/runtime/coroutines.hpp"

auto_drainer_t::auto_drainer_t() :
    refcount(0) { }

auto_drainer_t::~auto_drainer_t() {
    if (!draining.is_pulsed()) {
        begin_draining();
    }
    drained.wait_lazily_unordered();
    guarantee(refcount == 0);
}

auto_drainer_t::lock_t::lock_t() : parent(nullptr) {
}

auto_drainer_t::lock_t::lock_t(auto_drainer_t *p, throw_if_draining_t thr) : parent(p) {
    guarantee(parent != nullptr);
    if (thr == throw_if_draining_t::YES && parent->is_draining()) {
        throw interrupted_exc_t();
    }
    guarantee(!parent->is_draining(),
              "New processes should not acquire a draining `auto_drainer_t`.");
    parent->incref();
}

auto_drainer_t::lock_t::lock_t(const lock_t &l) : parent(l.parent) {
    if (parent) parent->incref();
}

auto_drainer_t::lock_t &auto_drainer_t::lock_t::operator=(const lock_t &l) {
    if (l.parent) l.parent->incref();
    if (parent) parent->decref();
    parent = l.parent;
    return *this;
}

auto_drainer_t::lock_t::lock_t(lock_t &&l) : parent(l.parent) {
    l.parent = nullptr;
}

auto_drainer_t::lock_t &auto_drainer_t::lock_t::operator=(lock_t &&l) {
    lock_t tmp(std::move(l));
    std::swap(parent, tmp.parent);
    return *this;
}

auto_drainer_t::lock_t auto_drainer_t::lock() {
    return auto_drainer_t::lock_t(this);
}

void auto_drainer_t::lock_t::reset() {
    if (parent) parent->decref();
    parent = nullptr;
}

bool auto_drainer_t::lock_t::has_lock() const {
    return parent != nullptr;
}


signal_t *auto_drainer_t::lock_t::get_drain_signal() const {
    guarantee(parent, "calling `get_drain_signal()` on a nil "
        "`auto_drainer_t::lock_t`.");
    return &parent->draining;
}

void auto_drainer_t::lock_t::assert_is_holding(auto_drainer_t *p) const {
    guarantee(p);
    guarantee(parent);
    guarantee(p == parent);
}

auto_drainer_t::lock_t::~lock_t() {
    if (parent) parent->decref();
}

void auto_drainer_t::begin_draining() {
    assert_not_draining();
    draining.pulse();
    if (refcount == 0) {
        drained.pulse();
    }
}

void auto_drainer_t::drain() {
    begin_draining();
    drained.wait_lazily_unordered();
    guarantee(refcount == 0);
}

void auto_drainer_t::incref() {
    assert_thread();
    refcount++;
}

void auto_drainer_t::decref() {
    assert_thread();
    refcount--;
    if (refcount == 0 && draining.is_pulsed()) {
        drained.pulse();
    }
}
