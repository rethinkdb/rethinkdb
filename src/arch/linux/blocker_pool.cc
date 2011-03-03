#include "arch/linux/blocker_pool.hpp"

// IO thread function
void* blocker_pool_t::event_loop(void *arg) {

    blocker_pool_t *parent = reinterpret_cast<blocker_pool_t*>(arg);

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
            system_mutex_t::lock_t ce_lock(&parent->ce_mutex);
            parent->completed_events.push_back(request);
            parent->ce_signal.write(1);
        }
    }
}

blocker_pool_t::blocker_pool_t(int nthreads, linux_event_queue_t *queue)
    : threads(nthreads), shutting_down(false), queue(queue)
{
    // Start the worker threads
    for (int i = 0; i < (int)threads.size(); i++) {
        int res = pthread_create(&threads[i], NULL,
            &blocker_pool_t::event_loop, reinterpret_cast<void*>(this));
        guarantee(res == 0, "Could not create blocker-pool thread.");
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
    for (int i = 0; i < (int)threads.size(); i++) {
        int res = pthread_join(threads[i], NULL);
        guarantee(res == 0, "Could not join blocker-pool thread.");
    }
}

void blocker_pool_t::do_job(job_t *job) {

    system_mutex_t::lock_t or_lock(&or_mutex);
    outstanding_requests.push_back(job);
    or_cond.signal();
}

void blocker_pool_t::on_event(int event) {

    rassert(event == poll_event_in);
    ce_signal.read();   // So pipe doesn't get backed up

    system_mutex_t::lock_t ce_lock(&ce_mutex);
    for (int i = 0; i < (int)completed_events.size(); i++) {
        completed_events[i]->done();
    }
    completed_events.clear();
}

