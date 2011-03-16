#ifndef __CONCURRENCY_DRAIN_SEMAPHORE_HPP__
#define __CONCURRENCY_DRAIN_SEMAPHORE_HPP__

#include "concurrency/cond_var.hpp"

/* A common paradigm is to have some resource and some number of processes using that
resource, and to wait for all the processes to complete before destroying the resource
but not to allow new processes to start. drain_semaphore_t encapsulates that. */

/* TODO, it might be nice if these were reusable */

struct drain_semaphore_t {

    drain_semaphore_t() : draining(false), refcount(0) { }
    ~drain_semaphore_t() {
        /* Should we assert draining here? Or should we call drain() if
        draining is false? Or is this good? */
        rassert(refcount == 0);
    }

    /* Each process should call acquire() when it begins and release() when it ends. */
    void acquire() {
        rassert(!draining);
        refcount++;
    }
    void release() {
        refcount--;
        if (draining && refcount == 0) cond.pulse();
    }

    /* Call drain() to wait for all processes to finish and not allow any new ones
    to start. */
    void drain() {
        draining = true;
        if (refcount) cond.wait();
    }

private:
    bool draining;
    int refcount;
    cond_t cond;
};

#endif /* __CONCURRENCY_DRAIN_SEMAPHORE_HPP__ */
