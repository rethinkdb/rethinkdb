#ifndef CONCURRENCY_CORO_POOL_HPP_
#define CONCURRENCY_CORO_POOL_HPP_

#include "errors.hpp"
#include <boost/bind.hpp>

#include "concurrency/auto_drainer.hpp"
#include "concurrency/queue/passive_producer.hpp"

/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t<boost::function<void()> >*`.

Right now, a number of different things depent on the `coro_pool_t` finishing
all of its tasks and draining its `passive_producer_t` before its destructor
returns. */

template <class T>
class coro_pool_t :
    private availability_callback_t,
    public home_thread_mixin_t
{
public:
    class callback_t {
    public:
        virtual ~callback_t() { }
        virtual void coro_pool_callback(T) = 0;
    };

    class boost_function_callback_t : public callback_t {
    public:
        boost_function_callback_t(boost::function<void(T)> _f)
            : f(_f)
        { }
        void coro_pool_callback(T t) {
            f(t);
        }
    private:
        boost::function<void(T)> f;
    };

    coro_pool_t(size_t _worker_count, passive_producer_t<T> *_source, callback_t *_callback)
        : max_worker_count(_worker_count),
          active_worker_count(0),
          source(_source),
          callback(_callback)
    {
        rassert(max_worker_count > 0);
        on_source_availability_changed();   // Start process if necessary
        source->available->set_callback(this);
    }

    ~coro_pool_t() {
        assert_thread();
        source->available->unset_callback();
    }

    void rethread(int new_thread) {
        /* Can't rethread while there are active operations */
        rassert(active_worker_count == 0);
        rassert(!source->available->get());
        real_home_thread = new_thread;
        coro_drain_semaphore.rethread(new_thread);
    }

private:
    void worker_run(UNUSED auto_drainer_t::lock_t coro_drain_semaphore_lock) {
        assert_thread();
        ++active_worker_count;
        rassert(active_worker_count <= max_worker_count);
        rassert(source->available->get());
        while (source->available->get()) {
            callback->coro_pool_callback(source->pop());
            assert_thread();
        }
        --active_worker_count;
    }

    void on_source_availability_changed() {
        assert_thread();
        while (source->available->get() && active_worker_count < max_worker_count) {
            coro_t::spawn_now(boost::bind(&coro_pool_t::worker_run,
                                          this,
                                          auto_drainer_t::lock_t(&coro_drain_semaphore)));
        }
    }

    int max_worker_count, active_worker_count;
    passive_producer_t<T> *source;
    callback_t *callback;
    auto_drainer_t coro_drain_semaphore;
};

#endif /* CONCURRENCY_CORO_POOL_HPP_ */

