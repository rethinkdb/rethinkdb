// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "errors.hpp"
#include <boost/bind.hpp>

#include "extproc/extproc_pool.hpp"
#include "extproc/extproc_worker.hpp"
#include "extproc/extproc_spawner.hpp"
#include "extproc/extproc_job.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/runtime/coroutines.hpp"

extproc_pool_t::extproc_pool_t(size_t worker_count) :
    available_worker_index(0),
    workers(worker_count)
{
    // The spawner should have been constructed outside of the thread pool
    extproc_spawner_t *spawner = extproc_spawner_t::get_instance();
    guarantee(spawner != NULL);

    // Construct cross-thread signals off of our interruptor for the workers
    ct_interruptors.init(get_num_threads());
    for (int i = 0; i < get_num_threads(); ++i) {
        ct_interruptors[i].init(new cross_thread_signal_t(&interruptor, i));
    }

    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i] = new extproc_worker_t(&ct_interruptors, spawner);
    }
}

extproc_pool_t::~extproc_pool_t() {
    // Can only be destructed on the same thread we were created on
    assert_thread();

    // Interrupt any and all workers
    interruptor.pulse();

    // Lock and delete all workers, running jobs should have been
    //  cancelled by the interruptor
    for (size_t i = 0; i < workers.size(); ++i) {
        extproc_worker_t *worker = acquire_internal(NULL);
        delete worker;
    }
}

// Class that allows us to cleanly get a worker when allocated on the stack
class extproc_pool_t::worker_request_t {
public:
    explicit worker_request_t(extproc_pool_t *_parent) :
        parent(_parent),
        worker_returned(false),
        request(new extproc_pool_t::request_node_t(&worker_promise))
    {
        parent->request_queue.push_back(request);
    }

    ~worker_request_t() {
        if (!worker_promise.is_pulsed()) {
            // No worker was given to us, just abandon the request
            request->abandon();
        } else if (!worker_returned) {
            // We were given a worker, but no one took it, release it
            extproc_worker_t *worker;
            bool gotten = worker_promise.try_get_value(&worker);
            guarantee(gotten);
            parent->release_internal(worker);
        }
    }

    extproc_worker_t *wait_and_get(signal_t *queue_interruptor) {
        extproc_worker_t *result = NULL;

        if (queue_interruptor == NULL) {
            worker_promise.wait();
        } else {
            wait_interruptible(worker_promise.get_ready_signal(), queue_interruptor);
        }

        bool gotten = worker_promise.try_get_value(&result);
        guarantee(gotten);

        worker_returned = true;
        return result;
    }

private:
    extproc_pool_t *parent;

    // Track whether we have given the worker to anyone.  If not, we have to re-release it
    bool worker_returned;
    promise_t<extproc_worker_t *> worker_promise;

    // The request in the request_queue.  We allocate this, but ownership is passed to
    //  the extproc_pool_t when we put it in the queue
    extproc_pool_t::request_node_t *request;
};

extproc_worker_t *extproc_pool_t::acquire_worker(signal_t *user_interruptor) {
    extproc_worker_t *worker;
    if (user_interruptor != NULL) {
        wait_any_t combined_interruptor(ct_interruptors[get_thread_id()].get(), user_interruptor);
        worker = acquire_internal(&combined_interruptor);
    } else {
        worker = acquire_internal(ct_interruptors[get_thread_id()].get());
    }
    worker->acquired(user_interruptor);
    return worker;
}

extproc_worker_t *extproc_pool_t::acquire_internal(signal_t *queue_interruptor) {
    system_mutex_t::lock_t lock(&worker_mutex);
    extproc_worker_t *result = NULL;

    if (available_worker_index == workers.size()) {
        worker_request_t request(this);
        lock.unlock();
        result = request.wait_and_get(queue_interruptor);
    } else {
        result = workers[available_worker_index];
        workers[available_worker_index] = NULL;
        ++available_worker_index;
    }

    guarantee(result != NULL);
    return result;
}

// Release will put the worker back into the table at index
void extproc_pool_t::release_worker(extproc_worker_t *worker) {
    worker->released();
    release_internal(worker);
}

void extproc_pool_t::release_internal(extproc_worker_t *worker) {
    system_mutex_t::lock_t lock(&worker_mutex);

    if (request_queue.size() > 0) {
        request_node_t *request = NULL;
        while (request_queue.size() > 0) {
            request = request_queue.head();
            request_queue.remove(request);

            if (!request->is_abandoned()) {
                break;
            }

            delete request;
            request = NULL;
        }

        if (request != NULL) {
            coro_t::spawn_sometime(boost::bind(&extproc_pool_t::pass_worker_coroutine,
                                               this, request, worker));
            return;
        }
    }

    // If we get here, there were no valid requests in the queue
    guarantee(available_worker_index > 0);
    guarantee(workers[available_worker_index - 1] == NULL);
    workers[available_worker_index - 1] = worker;
    --available_worker_index;
}

void extproc_pool_t::pass_worker_coroutine(request_node_t *request,
                                           extproc_worker_t *worker) {
    on_thread_t rethreader(request->get_thread());

    if (request->is_abandoned()) {
        // No one is waiting anymore, re-release the worker
        delete request;
        release_internal(worker);
    } else {
        request->fulfill_promise(worker);
        delete request;
    }
}

extproc_pool_t::request_node_t::request_node_t(promise_t<extproc_worker_t *> *_promise_out) :
    thread_id(get_thread_id()),
    promise_out(_promise_out) {
    guarantee(promise_out != NULL);
}

bool extproc_pool_t::request_node_t::is_abandoned() const {
    return promise_out == NULL;
}

int extproc_pool_t::request_node_t::get_thread() const {
    return thread_id;
}

void extproc_pool_t::request_node_t::abandon() {
    promise_out = NULL;
}

void extproc_pool_t::request_node_t::fulfill_promise(extproc_worker_t *worker) {
    guarantee(!is_abandoned());
    guarantee(get_thread_id() == thread_id);
    promise_out->pulse(worker);
}
