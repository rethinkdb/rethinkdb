#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "utils.hpp"
#include <boost/function.hpp>

#include "concurrency/queue/passive_producer.hpp"
#include "concurrency/drain_semaphore.hpp"



/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t<boost::function<void()> >*`.

Right now, a number of different things depent on the `coro_pool_t` finishing
all of its tasks and draining its `passive_producer_t` before its destructor
returns. */

class coro_t;

class coro_pool_t :
    public home_thread_mixin_t,
    private watchable_t<bool>::watcher_t
{
public:
    coro_pool_t(size_t worker_count_, passive_producer_t<boost::function<void()> > *source);

    ~coro_pool_t();
    void rethread(int new_thread);

    // Blocks until all pending tasks have been processed. The coro_pool_t is
    // reusable immediately after drain() returns.
    void drain();

private:
    passive_producer_t<boost::function<void()> > *source;

    void on_watchable_changed();

    int max_worker_count, active_worker_count;

    drain_semaphore_t coro_drain_semaphore;

    void worker_run(drain_semaphore_t::lock_t coro_drain_semaphore_lock);
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

