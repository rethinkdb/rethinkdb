#include "arch/runtime/runtime.hpp"

#include "utils.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/starter.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "do_on_thread.hpp"


int get_thread_id() {
    return linux_thread_pool_t::thread_id;
}

int get_num_threads() {
    return linux_thread_pool_t::thread_pool->n_threads;
}

#ifndef NDEBUG
void assert_good_thread_id(int thread) {
    rassertf(thread >= 0, "(thread = %d)", thread);
    rassertf(thread < get_num_threads(), "(thread = %d, n_threads = %d)", thread, get_num_threads());
}
#endif

bool continue_on_thread(int thread, linux_thread_message_t *msg) {
    assert_good_thread_id(thread);
    if (thread == linux_thread_pool_t::thread_id) {
        // The thread to continue on is the thread we are already on
        return true;
    } else {
        linux_thread_pool_t::thread->message_hub.store_message(thread, msg);
        return false;
    }
}

void call_later_on_this_thread(linux_thread_message_t *msg) {
    linux_thread_pool_t::thread->message_hub.store_message(linux_thread_pool_t::thread_id, msg);
}

struct starter_t : public thread_message_t {
    linux_thread_pool_t *tp;
    boost::function<void()> run;

    starter_t(linux_thread_pool_t *_tp, const boost::function<void()>& _fun) : tp(_tp), run(boost::bind(&starter_t::run_wrapper, this, _fun)) { }
    void on_thread_switch() {
        rassert(get_thread_id() == 0);
        coro_t::spawn_sometime(run);
    }
private:
    void run_wrapper(const boost::function<void()>& fun) {
        fun();
        tp->shutdown_thread_pool();
    }
};

// Runs the action 'fun()' on thread zero.
void run_in_thread_pool(const boost::function<void()>& fun, int worker_threads) {
    linux_thread_pool_t thread_pool(worker_threads, false);
    starter_t starter(&thread_pool, fun);
    thread_pool.run_thread_pool(&starter);
}
