// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_
#define CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_

#include "containers/scoped.hpp"
#include "containers/intrusive_list.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/promise.hpp"
#include "arch/io/concurrency.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/runtime/coroutines.hpp"

// This class implements a semaphore that can be used across threads
// Each element acquired from the semaphore is an item of type value_t, which may
//  be changed before being returned to the semaphore with `unlock()`
template <class value_t>
class cross_thread_semaphore_t {
public:
    // Constructs max_count elements with the given arguments
    template <class... Args>
    explicit cross_thread_semaphore_t(size_t max_count, Args&&... args) :
        available_value_index(0), values(max_count)
    {
        for (size_t i = 0; i < values.size(); ++i) {
            values[i] = new value_t(std::forward<Args>(args)...);
        }
    }

    // Acquires all outstanding elements before destruction completes
    ~cross_thread_semaphore_t();

    class lock_t {
    public:
        explicit lock_t(cross_thread_semaphore_t *_parent, signal_t *interruptor) :
            parent(_parent), value(parent->lock(interruptor)) { }

        ~lock_t() {
            parent->unlock(value);
        }

        value_t *get_value() { return value; }

    private:
        cross_thread_semaphore_t *parent;
        value_t *value;
        DISABLE_COPYING(lock_t);
    };

private:
    // Class used to queue up requests for items
    class request_node_t : public intrusive_list_node_t<request_node_t> {
    public:
        explicit request_node_t(promise_t<value_t *> *_promise_out) :
            thread_id(get_thread_id()),
            promise_out(_promise_out) {
            guarantee(promise_out != nullptr);
        }

        bool is_abandoned() const { return promise_out == nullptr; }
        threadnum_t get_thread() const { return thread_id; }
        void abandon() { promise_out = nullptr; }

        void fulfill_promise(value_t *value) {
            guarantee(!is_abandoned());
            guarantee(get_thread_id() == thread_id);
            promise_out->pulse(value);
        }

    private:
        threadnum_t thread_id;
        promise_t<value_t *> *promise_out;
        DISABLE_COPYING(request_node_t);
    };

    // Class that allows us to cleanly get a value when allocated on the stack
    class request_t {
    public:
        explicit request_t(cross_thread_semaphore_t *_parent);
        ~request_t();

        value_t *wait_and_get(signal_t *interruptor);

    private:
        cross_thread_semaphore_t *parent;

        // Track whether we have given the value to anyone.  If not, we have to re-release it
        bool value_returned;
        promise_t<value_t *> value_promise;

        // The request in the request_queue.  We allocate this, but ownership is passed to
        //  the parent when we put it in the queue
        request_node_t *request;
    };

    value_t *lock(signal_t *interruptor);
    void unlock(value_t *value);

    // Mutex to control access, since a lock may be constructed from any thread
    // The mutex is guaranteed to be released within constant time
    system_mutex_t mutex;

    // Coroutine to switch to the target thread and hand off the acquired item
    void pass_value_coroutine(request_node_t *request,
                              value_t *value);

    size_t available_value_index;
    scoped_array_t<value_t *> values;
    intrusive_list_t<request_node_t> request_queue;

    DISABLE_COPYING(cross_thread_semaphore_t);
};

// Acquires all outstanding elements before destruction completes
template <class value_t>
cross_thread_semaphore_t<value_t>::~cross_thread_semaphore_t() {
    for (size_t i = 0; i < values.size(); ++i) {
        delete lock(nullptr);
    }
}

template <class value_t>
void cross_thread_semaphore_t<value_t>::pass_value_coroutine(request_node_t *request,
                                                             value_t *value) {
    on_thread_t rethreader(request->get_thread());

    if (request->is_abandoned()) {
        // No one is waiting anymore, re-release the item
        delete request;
        unlock(value);
    } else {
        request->fulfill_promise(value);
        delete request;
    }
}

template <class value_t>
value_t *cross_thread_semaphore_t<value_t>::lock(signal_t *interruptor) {
    system_mutex_t::lock_t _lock(&mutex);
    value_t *result = nullptr;

    if (available_value_index == values.size()) {
        request_t request(this);
        _lock.unlock();
        result = request.wait_and_get(interruptor);
    } else {
        result = values[available_value_index];
        values[available_value_index] = nullptr;
        ++available_value_index;
    }

    guarantee(result != nullptr);
    return result;
}

template <class value_t>
void cross_thread_semaphore_t<value_t>::unlock(value_t *value) {
    system_mutex_t::lock_t _lock(&mutex);

    if (request_queue.size() > 0) {
        request_node_t *request = nullptr;
        while (request_queue.size() > 0) {
            request = request_queue.head();
            request_queue.remove(request);

            if (!request->is_abandoned()) {
                break;
            }

            delete request;
            request = nullptr;
        }

        if (request != nullptr) {
            coro_t::spawn_sometime(std::bind(&cross_thread_semaphore_t<value_t>::pass_value_coroutine,
                                             this, request, value));
            return;
        }
    }

    // If we get here, there were no valid requests in the queue
    guarantee(available_value_index > 0);
    guarantee(values[available_value_index - 1] == nullptr);
    values[available_value_index - 1] = value;
    --available_value_index;
}

template <class value_t>
cross_thread_semaphore_t<value_t>::request_t::request_t(cross_thread_semaphore_t *_parent) :
    parent(_parent),
    value_returned(false),
    request(new request_node_t(&value_promise))
{
    parent->request_queue.push_back(request);
}

template <class value_t>
cross_thread_semaphore_t<value_t>::request_t::~request_t() {
    if (!value_promise.is_pulsed()) {
        // No item was given to us, just abandon the request
        request->abandon();
    } else if (!value_returned) {
        // We were given an item, but no one took it, release it
        value_t *value;
        bool gotten = value_promise.try_get_value(&value);
        guarantee(gotten);
        parent->unlock(value);
    }
}

template <class value_t>
value_t *cross_thread_semaphore_t<value_t>::request_t::wait_and_get(signal_t *interruptor) {
    value_t *result = nullptr;

    if (interruptor == nullptr) {
        value_promise.wait();
    } else {
        wait_interruptible(value_promise.get_ready_signal(), interruptor);
    }

    bool gotten = value_promise.try_get_value(&result);
    guarantee(gotten);

    value_returned = true;
    return result;
}

#endif /* CONCURRENCY_CROSS_THREAD_SEMAPHORE_HPP_ */
