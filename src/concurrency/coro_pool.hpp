#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "errors.hpp"
#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/drain_semaphore.hpp"

/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t<boost::function<void()> >*`. */

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

private:
    passive_producer_t<boost::function<void()> > *source;

    void on_watchable_changed() {
        if (source->available->get() && active_worker_count < max_worker_count) {
            spin_up_new();
        }
    }

    int max_worker_count, active_worker_count;

    drain_semaphore_t coro_drain_semaphore;

    void spin_up_new() {
        rassert(active_worker_count < max_worker_count);
        coro_t::spawn_now(boost::bind(
            &coro_pool_t::worker_run,
                this,
                drain_semaphore_t::lock_t(&coro_drain_semaphore)
            ));
    }

    void worker_run(UNUSED drain_semaphore_t::lock_t coro_drain_semaphore_lock) {
        assert_thread();
        ++active_worker_count;
        while (source->available->get()) {
            /* Pop the task that we are going to do off the queue */
            boost::function<void()> task = source->pop();

            if (source->available->get() && active_worker_count < max_worker_count) {
                /* There are still tasks on the queue, and we haven't hit the
                worker limit yet, so there is benefit in spinning up another
                coroutine. */
                spin_up_new();
            }

            /* Perform the task */
            task();
            assert_thread(); // Make sure that `task()` didn't mess with us
        }
        --active_worker_count;
    }
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

