#include "concurrency/auto_drainer.hpp"

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

auto_drainer_t::lock_t::lock_t(auto_drainer_t *p) : parent(p) {
    rassert(!parent->draining.is_pulsed(), "New processes should not acquire "
        "a draining `auto_drainer_t`.");
    parent->incref();
}

auto_drainer_t::lock_t::lock_t(const lock_t &l) : parent(l.parent) {
    parent->incref();
}

auto_drainer_t::lock_t::lock_t &auto_drainer_t::lock_t::operator=(const lock_t &l) {
    l.parent->incref();
    parent->decref();
    parent = l.parent;
    return *this;
}

signal_t *auto_drainer_t::lock_t::get_drain_signal() {
    return &parent->draining;
}

auto_drainer_t::lock_t::~lock_t() {
    parent->decref();
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