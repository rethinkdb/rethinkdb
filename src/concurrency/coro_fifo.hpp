// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CORO_FIFO_HPP_
#define CONCURRENCY_CORO_FIFO_HPP_

#include "errors.hpp"
#include "concurrency/cond_var.hpp"
#include "containers/intrusive_list.hpp"

// A coro_fifo_t is an object that people on the same thread can
// acquire and release.  Multiple people can acquire it at the same
// time, but they have to release it in the same order in which they
// acquire it.  This could, for example, prevent sets from hopping
// over one another.

// This must run on a single thread, which limits its usefulness.

/* `coro_fifo_t` is obsolete. Use `fifo_enforcer_t` instead. `fifo_enforcer_t`
is safer because exiting the `fifo_enforcer_t` acquires a lock instead of just
assuming that nothing will block between exiting the `coro_fifo_t` and accessing
whatever the next important thing is. Also, `fifo_enforcer_t` can be split
across two different threads or machines. */

class coro_fifo_acq_t;

class coro_fifo_t : public home_thread_mixin_t {
public:
    coro_fifo_t() : queue_counter_(0) { }

    friend class coro_fifo_acq_t;

    void rethread(int new_home_thread) { real_home_thread = new_home_thread; }

private:
    void inform_ready_to_leave(coro_fifo_acq_t *acq);
    void inform_left_and_notified();
    void consider_pulse();

    // The acquisitor_queue_ is either empty, or it begins with a
    // coro_fifo_acq_t for which asked_to_leave_ is false.  When a
    // coro_fifo_acq_t is ready to die, it tells us so.  Then we set
    // its asked_to_leave_ to true and pulse the contiguous group
    // of coro_fifo_acq_t's at the front of the queue that have
    // asked_to_leave_ set to true.  This lets us shove stuff out
    // of the acquisitor_queue_ without waiting for another turn of
    // the event queue for every item we could remove.
    intrusive_list_t<coro_fifo_acq_t> acquisitor_queue_;

    // Counts how many coroutines have been pulse but have not yet woken.
    int64_t queue_counter_;

    DISABLE_COPYING(coro_fifo_t);
};

class coro_fifo_acq_t : public intrusive_list_node_t<coro_fifo_acq_t> {
public:
    friend class coro_fifo_t;

    coro_fifo_acq_t();
    ~coro_fifo_acq_t();
    void enter(coro_fifo_t *fifo);
    void leave();

private:
    bool initialized_;
    coro_fifo_t *fifo_;
    cond_t ready_to_leave_;
    bool asked_to_leave_;

    DISABLE_COPYING(coro_fifo_acq_t);
};


#endif  // CONCURRENCY_CORO_FIFO_HPP_
