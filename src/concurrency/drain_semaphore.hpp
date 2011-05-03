#ifndef __CONCURRENCY_DRAIN_SEMAPHORE_HPP__
#define __CONCURRENCY_DRAIN_SEMAPHORE_HPP__

#include "concurrency/resettable_cond_var.hpp"
#include "concurrency/pmap.hpp"
#include <boost/scoped_array.hpp>

/* A common paradigm is to have some resource and some number of processes using that
resource, and to wait for all the processes to complete before destroying the resource
but not to allow new processes to start. drain_semaphore_t encapsulates that. */

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
    void drain() {
        draining = true;
        if (refcount) {
            cond.get_signal()->wait();
            cond.reset();
        }
        draining = false;
    }

private:
    DISABLE_COPYING(drain_semaphore_t);
    bool draining;
    int refcount;
    resettable_cond_t cond;
};

/* threadsafe_drain_semaphore_t is like drain_semaphore_t but it can be safely used from
multiple threads. */

struct threadsafe_drain_semaphore_t {

    threadsafe_drain_semaphore_t() : sems(new drain_semaphore_t[get_num_threads()]) { }

    void acquire() {
        sems[get_thread_id()].acquire();
    }

    void release() {
        sems[get_thread_id()].release();
    }

    struct lock_t {
        lock_t(threadsafe_drain_semaphore_t *p) : lock(&p->sems[get_thread_id()]) { }
    private:
        drain_semaphore_t::lock_t lock;
    };

    void drain() {
        pmap(get_num_threads(), boost::bind(&threadsafe_drain_semaphore_t::subdrain, this, _1));
    }

private:
    void subdrain(int i) {
        on_thread_t thread_switcher(i);
        sems[i].drain();
    }

    DISABLE_COPYING(threadsafe_drain_semaphore_t);
    boost::scoped_array<drain_semaphore_t> sems;
};

#endif /* __CONCURRENCY_DRAIN_SEMAPHORE_HPP__ */
