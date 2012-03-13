#include "concurrency/coro_pool.hpp"

#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"
#include "concurrency/queue/passive_producer.hpp"

coro_pool_t::coro_pool_t(size_t worker_count_, availability_t * const available_) :
    available(available_),
    max_worker_count(worker_count_),
    active_worker_count(0)
{
    rassert(max_worker_count > 0);
    on_source_availability_changed();   // Start process if necessary
    available->set_callback(boost::bind(&coro_pool_t::on_source_availability_changed, this));
}

coro_pool_t::~coro_pool_t() {
    assert_thread();
    available->unset_callback();
    coro_drain_semaphore.drain();
    rassert(active_worker_count == 0);
}

void coro_pool_t::on_source_availability_changed() {
    assert_thread();
    while (available->get() && active_worker_count < max_worker_count) {
        coro_t::spawn_now(boost::bind(&coro_pool_t::worker_run,
                                      this,
                                      drain_semaphore_t::lock_t(&coro_drain_semaphore)));
    }
}

void coro_pool_t::worker_run(UNUSED drain_semaphore_t::lock_t coro_drain_semaphore_lock) {
    assert_thread();
    ++active_worker_count;
    rassert(active_worker_count <= max_worker_count);
    while (available->get()) {
        /* Pop the task that we are going to do off the queue */
        run_internal();
        assert_thread();   // Make sure that `task()` didn't mess with us
    }
    --active_worker_count;
}

void coro_pool_t::rethread(int new_thread) {
    /* Can't rethread while there are active operations */
    rassert(active_worker_count == 0);
    rassert(!available->get());
    real_home_thread = new_thread;
    coro_drain_semaphore.rethread(new_thread);
}

coro_pool_boost_t::coro_pool_boost_t(size_t worker_count_, passive_producer_t<boost::function<void()> > *source_) :
    coro_pool_t(worker_count_, source_->available),
    source(source_)
{ }

coro_pool_boost_t::~coro_pool_boost_t()
{ }

void coro_pool_boost_t::run_internal()
{
    source->pop()();
}
