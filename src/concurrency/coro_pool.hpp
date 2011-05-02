#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "errors.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/gate.hpp"
#include "concurrency/semaphore.hpp"

/* coro_pool_t allows tasks to be queued. These tasks are dispatched
to a number of coroutine workers. The workers themselves get allocated and destroyed
dynamically, obeying some limit on the maximal number of running worker coroutines.

TODO: coro_pool_t should be split. The part that actually dispatches the coroutines
should be an active consumer. The part that queues up the coroutines should be a
separate, generic queue object. */

class coro_pool_t : public home_thread_mixin_t {
public:
    coro_pool_t(size_t worker_count_, size_t max_queue_depth_) :
        queue_depth_sem(max_queue_depth_),
        max_worker_count(worker_count_),
        active_worker_count(0)
    {
        rassert(max_worker_count > 0);
    }

    ~coro_pool_t() {
        assert_thread();

        debugf("Begin ~coro_pool_t()\n");

        task_drain_semaphore.drain();
        coro_drain_semaphore.drain();

        debugf("End ~coro_pool_t()\n");

        rassert(task_queue.empty());
        rassert(active_worker_count == 0);
    }

    void queue_task(const boost::function<void()> task) {

        rassert(task);
        on_thread_t thread_switcher(home_thread);

        rassert(active_worker_count <= max_worker_count);

        // Place the task on the queue
        task_drain_semaphore.acquire();
        queue_depth_sem.co_lock();
        task_queue.push_back(task);

        // Spawn a new worker if permitted
        if (active_worker_count < max_worker_count) {
            ++active_worker_count;
            coro_t::spawn(boost::bind(
                &coro_pool_t::worker_run,
                    this,
                    drain_semaphore_t::lock_t(&coro_drain_semaphore)
                ));
        }
    }

private:
    semaphore_t queue_depth_sem;

    int max_worker_count;
    int active_worker_count;
    std::deque<boost::function<void()> > task_queue;

    drain_semaphore_t task_drain_semaphore, coro_drain_semaphore;

    void worker_run(UNUSED drain_semaphore_t::lock_t coro_drain_semaphore_lock) {
        assert_thread();
        while (!task_queue.empty()) {
            boost::function<void()> task = task_queue.front();
            task_queue.pop_front();
            task();
            debugf("Done task.\n");
            queue_depth_sem.unlock();
            task_drain_semaphore.release();
            assert_thread(); // Make sure that task() didn't mess with us
        }
        --active_worker_count;
    }
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

