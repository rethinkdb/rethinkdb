#ifndef __CONCURRENCY_CORO_POOL_HPP__
#define	__CONCURRENCY_CORO_POOL_HPP__

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
            shutting_down(false) {

        rassert(max_worker_count > 0);

        worker_active_gate_opener.reset(new gate_t::open_t(&worker_active_gate));
    }
    ~coro_pool_t() {
        assert_thread();
        shutting_down = true;

        // Wait till queued tasks get handled...
        // (in case queue_task got called and spawned a new coroutine but that coroutine has not materialized yet)
        if (!task_queue.empty()) {
            coro_t::yield();
        }

        // Close the active gate. This automatically waits on all workers to finish whatever they are doing
        worker_active_gate_opener.reset();

        rassert(task_queue.empty());
        rassert(active_worker_count == 0);
    }

    // Must be called on home_thread. Use queue_task_on_home_thread if calling from a different thread
    void queue_task(const boost::function<void()> &task) {
        assert_thread();
        rassert(!shutting_down);
        rassert(active_worker_count <= max_worker_count);

        // Place the task on the queue
        task_queue.push_back(task);

        // Spawn a new worker if permitted
        if (active_worker_count < max_worker_count) {
            ++active_worker_count;
            coro_t::spawn(boost::bind(&coro_pool_t::worker_run, this));
        }
    }

    void queue_task_on_home_thread(const boost::function<void()> &task) {
        do_on_thread(home_thread, boost::bind(&coro_pool_t::queue_task, this, task));
    }

private:
    int max_worker_count;
    int active_worker_count;
    std::deque<boost::function<void()> > task_queue;
    bool shutting_down;
    gate_t worker_active_gate;
    boost::scoped_ptr<gate_t::open_t> worker_active_gate_opener;

    void worker_run() {
        assert_thread();
        rassert(worker_active_gate.is_open()); // The gate should be open as long as we exist
        gate_t::entry_t active(&worker_active_gate);
        while (!task_queue.empty()) {
            boost::function<void()> task = task_queue.front();
            task_queue.pop_front();
            task();
            assert_thread(); // Make sure that task() didn't mess with us
        }
        --active_worker_count;
    }
};

#endif	/* __CONCURRENCY_CORO_POOL_HPP__ */

