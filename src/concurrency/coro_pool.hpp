// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONCURRENCY_CORO_POOL_HPP_
#define CONCURRENCY_CORO_POOL_HPP_

#include <functional>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/queue/passive_producer.hpp"

/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t`. */

template <class T>
class coro_pool_callback_t {
public:
    virtual ~coro_pool_callback_t() { }
    virtual void coro_pool_callback(T, signal_t *) = 0;
};

template <class T>
class std_function_callback_t : public coro_pool_callback_t<T> {
public:
    explicit std_function_callback_t(std::function<void(T, signal_t *)> _f)
        : f(std::move(_f))
    { }
    void coro_pool_callback(T t, signal_t *interruptor) {
        f(t, interruptor);
    }
private:
    std::function<void(T, signal_t *)> f;

    DISABLE_COPYING(std_function_callback_t);
};

template <class T>
class coro_pool_t : private availability_callback_t, public home_thread_mixin_t {
public:
    coro_pool_t(size_t _worker_count, passive_producer_t<T> *_source, coro_pool_callback_t<T> *_callback)
        : max_worker_count(_worker_count),
          active_worker_count(0),
          source(_source),
          callback(_callback) {
        rassert(max_worker_count > 0);
        on_source_availability_changed();   // Start process if necessary
        source->available->set_callback(this);
    }

    ~coro_pool_t() {
        assert_thread();
        source->available->unset_callback();
    }

    void rethread(threadnum_t new_thread) {
        /* Can't rethread while there are active operations */
        rassert(active_worker_count == 0);
        rassert(!source->available->get());
        real_home_thread = new_thread;
        coro_drain_semaphore.rethread(new_thread);
    }

private:
    void worker_run(T object, auto_drainer_t::lock_t coro_drain_semaphore_lock) THROWS_NOTHING {
        assert_thread();
        try {
            while (!coro_drain_semaphore_lock.get_drain_signal()->is_pulsed()) {
                callback->coro_pool_callback(object, coro_drain_semaphore_lock.get_drain_signal());
                if (source->available->get()) {
                    object = source->pop();
                } else {
                    break;
                }
                coro_t::yield();
            }
        } catch (const interrupted_exc_t &) {
            rassert(coro_drain_semaphore_lock.get_drain_signal()->is_pulsed());
        }
        --active_worker_count;
    }

    void on_source_availability_changed() {
        assert_thread();
        while (source->available->get() && active_worker_count < max_worker_count) {
            ++active_worker_count;
            coro_t::spawn_sometime(std::bind(
                &coro_pool_t::worker_run, this,
                source->pop(), auto_drainer_t::lock_t(&coro_drain_semaphore)));
        }
    }

    int max_worker_count, active_worker_count;
    passive_producer_t<T> *source;
    coro_pool_callback_t<T> *callback;
    auto_drainer_t coro_drain_semaphore;
};

class calling_callback_t : public coro_pool_callback_t<std::function<void()> > {
public:
    void coro_pool_callback(std::function<void()> f, UNUSED signal_t *interruptor) {
        f();
    }
};

#endif /* CONCURRENCY_CORO_POOL_HPP_ */

