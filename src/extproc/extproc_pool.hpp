// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef EXTPROC_EXTPROC_POOL_HPP_
#define EXTPROC_EXTPROC_POOL_HPP_

#include "utils.hpp"
#include "containers/scoped.hpp"
#include "containers/intrusive_list.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/promise.hpp"
#include "arch/io/concurrency.hpp"

class extproc_worker_t;

// Extproc pool is used to acquire and release workers from any thread,
//  must be created from within the thread pool
class extproc_pool_t : public home_thread_mixin_t {
public:
    explicit extproc_pool_t(size_t worker_count);
    ~extproc_pool_t();

    extproc_worker_t *acquire_worker(signal_t *interruptor);
    void release_worker(extproc_worker_t *worker);

private:
    extproc_worker_t *acquire_internal(signal_t *user_interruptor);
    void release_internal(extproc_worker_t *worker);

    class worker_request_t;

    // Class used to queue up requests for workers
    class request_node_t : public intrusive_list_node_t<request_node_t> {
    public:
        explicit request_node_t(promise_t<extproc_worker_t *> *_promise_out);

        bool is_abandoned() const;
        int get_thread() const;

        void abandon();

        void fulfill_promise(extproc_worker_t *worker);

    private:
        // The thread that the request is for, have to switch here to fulfill
        int thread_id;

        // Where to store the worker when we have it
        promise_t<extproc_worker_t *> *promise_out;
    };

    void pass_worker_coroutine(request_node_t *request,
                               extproc_worker_t *worker);

    // Mutex to control access to 'workers' and 'request_queue'
    system_mutex_t worker_mutex;

    // Pointers to all workers available in this instance, when a worker is taken, it
    //  will be replaced by NULL in the array, and replaced when done
    size_t available_worker_index;
    scoped_array_t<extproc_worker_t *> workers;

    // The list of requests (added from any thread, protected by worker_mutex)
    intrusive_list_t<request_node_t> request_queue;

    // The interruptor to be pulsed when shutting down
    cond_t interruptor;

    // Crossthreaded interruptors to notify workers on any thread when we are shutting down
    scoped_array_t<scoped_ptr_t<cross_thread_signal_t> > ct_interruptors;
};

#endif /* EXTPROC_EXTPROC_POOL_HPP_ */
