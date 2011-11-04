#ifndef __CONCURRENCY_DRAIN_SEMAPHORE_HPP__
#define __CONCURRENCY_DRAIN_SEMAPHORE_HPP__

#include "concurrency/resettable_cond_var.hpp"
#include "utils.hpp"

/* A common paradigm is to have some resource and some number of processes using
that resource, and to wait for all the processes to complete before destroying
the resource but not to allow new processes to start. `drain_semaphore_t`
encapsulates that.

If you want the same thing except reusable, look at `gate_t`.

WARNING: You should probably use `auto_drainer_t` instead of
`drain_semaphore_t`. They serve similar purposes, but `auto_drainer_t` is better
and it would be nice to get rid of `drain_semaphore_t` completely. */

struct drain_semaphore_t : public home_thread_mixin_t {

    explicit drain_semaphore_t(int specified_home_thread);
    drain_semaphore_t();
    ~drain_semaphore_t();

    /* Each process should call acquire() when it begins and release() when it ends. */
    void acquire();
    void release();

    struct lock_t {
        explicit lock_t(drain_semaphore_t *p);
        lock_t(const lock_t& copy_me);
        lock_t &operator=(const lock_t &copy_me);
        ~lock_t();
    private:
        drain_semaphore_t *parent;
    };

    /* Call drain() to wait for all processes to finish and not allow any new ones
    to start. */
    void drain();

    void rethread(int new_thread);

private:
    bool draining;
    int refcount;
    resettable_cond_t cond;

    DISABLE_COPYING(drain_semaphore_t);
};

#endif /* __CONCURRENCY_DRAIN_SEMAPHORE_HPP__ */
