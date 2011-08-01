#include "concurrency/drain_semaphore.hpp"
#include "concurrency/signal.hpp"


drain_semaphore_t::drain_semaphore_t() : draining(false), refcount(0) { }
drain_semaphore_t::~drain_semaphore_t() {
    /* Should we assert draining here? Or should we call drain() if
       draining is false? Or is this good? */
    rassert(refcount == 0);
}

/* Each process should call acquire() when it begins and release() when it ends. */
void drain_semaphore_t::acquire() {
    assert_thread();
    rassert(!draining);
    refcount++;
}
void drain_semaphore_t::release() {
    assert_thread();
    refcount--;
    if (draining && refcount == 0) {
        cond.pulse();
    }
}

drain_semaphore_t::lock_t::lock_t(drain_semaphore_t *p) : parent(p) {
    parent->acquire();
}

drain_semaphore_t::lock_t::lock_t(const lock_t& copy_me) : parent(copy_me.parent) {
    parent->refcount++;
}

drain_semaphore_t::lock_t& drain_semaphore_t::lock_t::operator=(const lock_t &copy_me) {
    // We have to increment the reference count before calling release()
    // in case `parent` is already `copy_me.parent`
    copy_me.parent->refcount++;
    parent->release();
    parent = copy_me.parent;
    return *this;
}

drain_semaphore_t::lock_t::~lock_t() {
    parent->release();
}

void drain_semaphore_t::rethread(int new_thread) {
    rassert(refcount == 0);
    real_home_thread = new_thread;
}


void drain_semaphore_t::drain() {
    draining = true;
    if (refcount) {
        cond.get_signal()->wait();
        cond.reset();
    }
    draining = false;
}
