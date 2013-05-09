// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "concurrency/auto_drainer.hpp"

#include "arch/runtime/coroutines.hpp"

auto_drainer_t::auto_drainer_t() :
    refcount(0), when_done(NULL) { }

auto_drainer_t::~auto_drainer_t() {
    if (refcount != 0) {
        when_done = coro_t::self();
        draining.pulse();
        coro_t::wait();
    }
    rassert(refcount == 0);
}

auto_drainer_t::lock_t::lock_t() : parent(NULL) {
}

auto_drainer_t::lock_t::lock_t(auto_drainer_t *p) : parent(p) {
    rassert(parent != NULL);
    rassert(!parent->draining.is_pulsed(), "New processes should not acquire "
        "a draining `auto_drainer_t`.");
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

void auto_drainer_t::lock_t::reset() {
    if (parent) parent->decref();
    parent = NULL;
}

signal_t *auto_drainer_t::lock_t::get_drain_signal() const {
    rassert(parent, "calling `get_drain_signal()` on a nil "
        "`auto_drainer_t::lock_t`.");
    return &parent->draining;
}

void auto_drainer_t::lock_t::assert_is_holding(DEBUG_VAR auto_drainer_t *p) {
    rassert(p);
    rassert(parent);
    rassert(p == parent);
}

auto_drainer_t::lock_t::~lock_t() {
    if (parent) parent->decref();
}

void auto_drainer_t::incref() {
    assert_thread();
    refcount++;
}

void auto_drainer_t::decref() {
    assert_thread();
    refcount--;
    if (refcount == 0 && draining.is_pulsed()) {
        when_done->notify_sometime();
    }
}
