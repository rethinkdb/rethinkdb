#include "arch/io/blocker_pool.hpp"

#include <string.h>

__thread int thread_is_blocker_pool_thread = 0;

// IO thread function
void* blocker_pool_t::event_loop(void *arg) {

    thread_is_blocker_pool_thread = 1;

    blocker_pool_t *parent = reinterpret_cast<blocker_pool_t*>(arg);

    // Disable signals on this thread. This ensures that signals like SIGINT are
    // handled by one of the main threads.
    {
        sigset_t sigmask;
        int res = sigfillset(&sigmask);
        guaranteef_err(res == 0, "Could not get a full sigmask");

        res = pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
        guaranteef_err(res == 0, "Could not block signal");
    }

    while (true) {
        // Wait for an IO command or shutdown command. The while-loop guards against spurious
        // wakeups.
        system_mutex_t::lock_t or_lock(&parent->or_mutex);
        while (parent->outstanding_requests.empty() && !parent->shutting_down) {
            parent->or_cond.wait(&parent->or_mutex);
        }

        if (parent->shutting_down) {
            // or_lock's destructor gets called, so everything is OK.
            return NULL;

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
            parent->ce_signal.write(1);
        }
    }
}

blocker_pool_t::blocker_pool_t(int nthreads, linux_event_queue_t *_queue)
    : threads(nthreads), shutting_down(false), queue(_queue)
{
    // Start the worker threads
    for (size_t i = 0; i < threads.size(); ++i) {
        int res = pthread_create(&threads[i], NULL,
            &blocker_pool_t::event_loop, reinterpret_cast<void*>(this));
        guaranteef(res == 0, "Could not create blocker-pool thread.");
    }

    // Register with event queue so we get the completion events
    queue->watch_resource(ce_signal.get_notify_fd(), poll_event_in, this);
}

blocker_pool_t::~blocker_pool_t() {

    // Deregister with the event queue
    queue->forget_resource(ce_signal.get_notify_fd(), this);

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
        int res = pthread_join(threads[i], NULL);
        guaranteef(res == 0, "Could not join blocker-pool thread.");
    }
}

void blocker_pool_t::do_job(job_t *job) {

    system_mutex_t::lock_t or_lock(&or_mutex);
    outstanding_requests.push_back(job);
    or_cond.signal();
}

void blocker_pool_t::on_event(DEBUG_VAR int event) {

    rassert(event == poll_event_in);
    ce_signal.read();   // So pipe doesn't get backed up

    std::vector<job_t *> local_completed_events;

    {
        system_mutex_t::lock_t ce_lock(&ce_mutex);
        local_completed_events.swap(completed_events);
    }

    for (size_t i = 0; i < local_completed_events.size(); ++i) {
        local_completed_events[i]->done();
    }
}



bool i_am_in_blocker_pool_thread() {
    return thread_is_blocker_pool_thread == 1;
}
