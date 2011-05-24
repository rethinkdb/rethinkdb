#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "errors.hpp"
#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/drain_semaphore.hpp"

/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t<boost::function<void()> >*`.

Right now, a number of different things depent on the `coro_pool_t` finishing
all of its tasks and draining its `passive_producer_t` before its destructor
returns. */

class coro_pool_t :
    public home_thread_mixin_t,
    private watchable_t<bool>::watcher_t
{
public:
    coro_pool_t(size_t worker_count_, passive_producer_t<boost::function<void()> > *source) :
        source(source),
        max_worker_count(worker_count_),
        active_worker_count(0)
    {
        rassert(max_worker_count > 0);
        on_watchable_changed();   // Start process if necessary
        source->available->add_watcher(this);
    }

    ~coro_pool_t() {
        assert_thread();
        source->available->remove_watcher(this);
        coro_drain_semaphore.drain();
        rassert(active_worker_count == 0);
    }

    void rethread(int new_thread) {
        /* Can't rethread while there are active operations */
        rassert(active_worker_count == 0);
        rassert(!source->available->get());
        real_home_thread = new_thread;
    }

    // Blocks until all pending tasks have been processed. The coro_pool_t is
    // reusable immediately after drain() returns.
    void drain() {
        assert_thread();
        coro_drain_semaphore.drain();
    }

private:
    passive_producer_t<boost::function<void()> > *source;

    void on_watchable_changed() {
        assert_thread();
        while (source->available->get() && active_worker_count < max_worker_count) {
            coro_t::spawn_now(boost::bind(
                &coro_pool_t::worker_run,
                    this,
                    drain_semaphore_t::lock_t(&coro_drain_semaphore)
                ));
        }
    }

    int max_worker_count, active_worker_count;

    drain_semaphore_t coro_drain_semaphore;

    void worker_run(UNUSED drain_semaphore_t::lock_t coro_drain_semaphore_lock) {
        assert_thread();
        ++active_worker_count;
        rassert(active_worker_count <= max_worker_count);
        while (source->available->get()) {
            /* Pop the task that we are going to do off the queue */
            boost::function<void()> task = source->pop();
            task();   // Perform the task
            assert_thread();   // Make sure that `task()` didn't mess with us
        }
        --active_worker_count;
    }
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

