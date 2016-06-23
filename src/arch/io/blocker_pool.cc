// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/io/blocker_pool.hpp"

#include <string.h>

#include "arch/compiler.hpp"
#include "config/args.hpp"
#include "utils.hpp"

THREAD_LOCAL int thread_is_blocker_pool_thread = 0;
// Access functions to thread_is_blocker_pool_thread. Marked as NOINLINE
// to avoid certain compiler optimizations that can break TLS. See
// thread_local.hpp for a more detailed explanation.
NOINLINE void set_in_blocker_pool_thread(int value) {
    thread_is_blocker_pool_thread = value;
}
NOINLINE bool i_am_in_blocker_pool_thread() {
    return thread_is_blocker_pool_thread == 1;
}
// Make sure thread_is_blocker_pool_thread is not accessed directly
#ifdef __GNUC__
#pragma GCC poison thread_is_blocker_pool_thread
#endif

// IO thread function
void* blocker_pool_t::event_loop(void *arg) {

    set_in_blocker_pool_thread(1);

    blocker_pool_t *parent = reinterpret_cast<blocker_pool_t*>(arg);

#ifndef _WIN32
    // Disable signals on this thread. This ensures that signals like SIGINT are
    // handled by one of the main threads.
    {
        sigset_t sigmask;
        int res = sigfillset(&sigmask);
        guarantee_err(res == 0, "Could not get a full sigmask");

        res = pthread_sigmask(SIG_SETMASK, &sigmask, nullptr);
        guarantee_xerr(res == 0, res, "Could not block signal");
    }
#endif

    while (true) {
        // Wait for an IO command or shutdown command. The while-loop guards against spurious
        // wakeups.
        system_mutex_t::lock_t or_lock(&parent->or_mutex);
        while (parent->outstanding_requests.empty() && !parent->shutting_down) {
            parent->or_cond.wait(&parent->or_mutex);
        }

        if (parent->shutting_down) {
            // or_lock's destructor gets called, so everything is OK.
            return nullptr;

        } else {

            // Grab a request
            job_t *request = parent->outstanding_requests.front();
            parent->outstanding_requests.erase(parent->outstanding_requests.begin(),
                                               parent->outstanding_requests.begin() + 1);

            // Unlock the mutex manually instead of waiting for its destructor so that other jobs
            // will get to proceed
            or_lock.unlock();

            // Perform the request. It may block. This is the raison d'etre for blocker_pool_t.
            request->run();

            // Notify that the request is done
            {
                system_mutex_t::lock_t ce_lock(&parent->ce_mutex);
                parent->completed_events.push_back(request);
            }
            // It seems critical for performance that we release ce_lock *before* we write to the signal!
            // This is probably because the kernel thinks "hey, somebody is blocking on the signal.
            // Now that it has been written to, that thread will want to handle it, so let's wake it up."
            // If we don't have ce_lock released at that point, the following happens:
            // The signalled thread immediately has to acquire ce_lock, so the kernel has to yield control
            // again. Only after an additional scheduler roundtrip, we get control again and can release the ce_lock.
            parent->ce_signal.wakey_wakey();
        }
    }
}

blocker_pool_t::blocker_pool_t(int nthreads, linux_event_queue_t *_queue)
    : threads(nthreads), shutting_down(false), queue(_queue)
{
    // Start the worker threads
    for (size_t i = 0; i < threads.size(); ++i) {
        pthread_attr_t attr;
        int res = pthread_attr_init(&attr);
        guarantee_xerr(res == 0, res, "pthread_attr_init failed.");

        // The coroutine stack size should be enough for blocker pool stacks.  Right
        // now that's 128 KB.
        static_assert(COROUTINE_STACK_SIZE == 131072,
                      "Expecting COROUTINE_STACK_SIZE to be 131072.  If you changed "
                      "it, please double-check whether the value is appropriate for "
                      "blocker pool threads.");
        // Disregard failure -- we'll just use the default stack size if this somehow
        // fails.
        UNUSED int ignored_res = pthread_attr_setstacksize(&attr, COROUTINE_STACK_SIZE);

        res = pthread_create(&threads[i], &attr,
            &blocker_pool_t::event_loop, reinterpret_cast<void*>(this));
        guarantee_xerr(res == 0, res, "Could not create blocker-pool thread.");

        res = pthread_attr_destroy(&attr);
        guarantee_xerr(res == 0, res, "pthread_attr_destroy failed.");
    }

    // Register with event queue so we get the completion events
    queue->watch_event(&ce_signal, this);
}

blocker_pool_t::~blocker_pool_t() {

    // Deregister with the event queue
    queue->forget_event(&ce_signal, this);

    /* Send out the order to shut down */
    {
        system_mutex_t::lock_t or_lock(&or_mutex);

        shutting_down = true;

        /* It is an error to shut down the blocker pool while requests are still out */
        rassert(outstanding_requests.size() == 0);

        or_cond.broadcast();
    }

    /* Wait for stuff to actually shut down */
    for (size_t i = 0; i < threads.size(); ++i) {
        int res = pthread_join(threads[i], nullptr);
        guarantee_xerr(res == 0, res, "Could not join blocker-pool thread.");
    }
}

void blocker_pool_t::do_job(job_t *job) {

    system_mutex_t::lock_t or_lock(&or_mutex);
    outstanding_requests.push_back(job);
    or_cond.signal();
}

void blocker_pool_t::on_event(DEBUG_VAR int event) {

    rassert(event == poll_event_in);
    ce_signal.consume_wakey_wakeys();   // So pipe doesn't get backed up

    std::vector<job_t *> local_completed_events;

    {
        system_mutex_t::lock_t ce_lock(&ce_mutex);
        local_completed_events.swap(completed_events);
    }

    for (size_t i = 0; i < local_completed_events.size(); ++i) {
        local_completed_events[i]->done();
    }
}

