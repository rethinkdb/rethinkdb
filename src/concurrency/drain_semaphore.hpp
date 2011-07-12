#ifndef __CONCURRENCY_DRAIN_SEMAPHORE_HPP__
#define __CONCURRENCY_DRAIN_SEMAPHORE_HPP__

#include "utils.hpp"
#include "concurrency/resettable_cond_var.hpp"

/* A common paradigm is to have some resource and some number of processes using that
resource, and to wait for all the processes to complete before destroying the resource
but not to allow new processes to start. `drain_semaphore_t` encapsulates that.

If you want the same thing except reusable, look at `gate_t`. */

struct drain_semaphore_t : public home_thread_mixin_t {

    drain_semaphore_t() : draining(false), refcount(0) { }
    ~drain_semaphore_t() {
        /* Should we assert draining here? Or should we call drain() if
        draining is false? Or is this good? */
        rassert(refcount == 0);
    }

    /* Each process should call acquire() when it begins and release() when it ends. */
    void acquire() {
        assert_thread();
        rassert(!draining);
        refcount++;
    }
    void release() {
        assert_thread();
        refcount--;
        if (draining && refcount == 0) {
            cond.pulse();
        }
    }

    struct lock_t {
        lock_t(drain_semaphore_t *p) : parent(p) {
            parent->acquire();
        }
        lock_t(const lock_t& copy_me) : parent(copy_me.parent) {
            parent->refcount++;
        }
        lock_t &operator=(const lock_t &copy_me) {
            // We have to increment the reference count before calling release()
            // in case `parent` is already `copy_me.parent`
            copy_me.parent->refcount++;
            parent->release();
            parent = copy_me.parent;
            return *this;
        }
        ~lock_t() {
            parent->release();
        }
    private:
        drain_semaphore_t *parent;
    };

    /* Call drain() to wait for all processes to finish and not allow any new ones
    to start. */
    void drain();

    void rethread(int new_thread) {
        rassert(refcount == 0);
        real_home_thread = new_thread;
    }

private:
    DISABLE_COPYING(drain_semaphore_t);
    bool draining;
    int refcount;
    resettable_cond_t cond;
};

#endif /* __CONCURRENCY_DRAIN_SEMAPHORE_HPP__ */
