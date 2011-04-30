#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define __CONCURRENCY_CORO_POOL_HPP__

#include "errors.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/gate.hpp"


/* coro_pool_t allows tasks to be queued. These tasks are dispatched
to a number of coroutine workers. The workers themselves get allocated and destroyed
dynamically, obeying some limit on the maximal number of running worker coroutines. */

class coro_pool_t : public home_thread_mixin_t {
public:
    coro_pool_t(size_t worker_count_) :
        max_worker_count(worker_count_),
        active_worker_count(0),
        task_queue(worker_count_ * 10)
    {
        rassert(max_worker_count > 0);
    }

    ~coro_pool_t() {
        assert_thread();

        task_drain_semaphore.drain();
        coro_drain_semaphore.drain();

        rassert(task_queue.empty());
        rassert(active_worker_count == 0);
    }

    void queue_task(const boost::function<void()> &task) {

        if (get_thread_id() != home_thread) {
            do_on_thread(home_thread, boost::bind(&coro_pool_t::queue_task, this, task));

        } else {
            rassert(active_worker_count <= max_worker_count);

            // Place the task on the queue
            task_queue.push_back(task);
            task_drain_semaphore.acquire();

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
    }

private:
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
            task_drain_semaphore.release();
            assert_thread(); // Make sure that task() didn't mess with us
        }
        --active_worker_count;
    }
};

#endif /* __CONCURRENCY_CORO_POOL_HPP__ */

