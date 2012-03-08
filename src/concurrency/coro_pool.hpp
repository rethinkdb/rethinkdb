#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "utils.hpp"
#include <boost/function.hpp>

#include "concurrency/drain_semaphore.hpp"
#include "concurrency/queue/passive_producer.hpp"

/* coro_pool_t maintains a bunch of coroutines; when you give it tasks, it
distributes them among the coroutines. It draws its tasks from a
`passive_producer_t<boost::function<void()> >*`.

Right now, a number of different things depent on the `coro_pool_t` finishing
all of its tasks and draining its `passive_producer_t` before its destructor
returns. */

class coro_pool_t :
    public home_thread_mixin_t
{
public:
    coro_pool_t(size_t worker_count_, availability_t * const available_);
    virtual ~coro_pool_t();

    // Blocks until all pending tasks have been processed. The coro_pool_t is
    // reusable immediately after drain() returns.
    void drain();

protected:
    // Callback to be overloaded by derived classes when an item is available
    virtual void run_internal() = 0;

private:
    void worker_run(drain_semaphore_t::lock_t coro_drain_semaphore_lock);
    void on_source_availability_changed();

    availability_t * const available;
    int max_worker_count, active_worker_count;
    drain_semaphore_t coro_drain_semaphore;
};

class coro_pool_boost_t :
    public coro_pool_t
{
private:
    passive_producer_t<boost::function<void()> > *source;
    void run_internal();

public:
    coro_pool_boost_t(size_t worker_count_, passive_producer_t<boost::function<void()> > *source_);
    ~coro_pool_boost_t();
};

template <class Param>
class coro_pool_caller_t :
    public coro_pool_t
{
public:
    class callback_t {
    public:
        virtual ~callback_t() { }
        virtual void coro_pool_callback(Param) = 0;
    };

private:
    passive_producer_t<Param> *source;
    callback_t *callback_object;

    void run_internal() {
        callback_object->coro_pool_callback(source->pop());
    }

public:
    coro_pool_caller_t(size_t worker_count_,
                         passive_producer_t<Param> *source_,
                         callback_t *instance) :
        coro_pool_t(worker_count_, source_->available),
        source(source_),
        callback_object(instance)
    { }

    ~coro_pool_caller_t()
    { }
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

