// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/runtime/runtime.hpp"

#include <inttypes.h>

#include <functional>

#include "arch/runtime/starter.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "do_on_thread.hpp"

bool in_thread_pool() {
    return linux_thread_pool_t::get_thread_pool() != nullptr;
}

threadnum_t get_thread_id() {
    if (i_am_in_blocker_pool_thread()) {
        return threadnum_t(-1);
    }
    return threadnum_t(linux_thread_pool_t::get_thread_id());
}

int get_num_threads() {
    return linux_thread_pool_t::get_thread_pool()->n_threads;
}

#ifndef NDEBUG
void assert_good_thread_id(threadnum_t thread) {
    if (linux_thread_pool_t::get_thread_pool() == nullptr) {
        rassert(thread.threadnum == 0);
    } else {
        rassert(thread.threadnum >= 0, "(thread = %" PRIi32 ")", thread.threadnum);
        rassert(thread.threadnum < get_num_threads(), "(thread = %" PRIi32 ", n_threads = %d)",
                thread.threadnum, get_num_threads());
    }
}
#endif

bool continue_on_thread(threadnum_t thread, linux_thread_message_t *msg) {
    assert_good_thread_id(thread);
    if (thread.threadnum == linux_thread_pool_t::get_thread_id()) {
        // The thread to continue on is the thread we are already on
        return true;
    } else {
        linux_thread_pool_t::get_thread()->message_hub.store_message_ordered(thread, msg);
        return false;
    }
}

void call_later_on_this_thread(linux_thread_message_t *msg) {
    linux_thread_pool_t::get_thread()->message_hub.store_message_ordered(
        threadnum_t(linux_thread_pool_t::get_thread_id()),
        msg);
}

struct starter_t : public thread_message_t {
    linux_thread_pool_t *tp;
    std::function<void()> run;

    starter_t(linux_thread_pool_t *_tp, const std::function<void()> &_fun)
      : tp(_tp), run(std::bind(&starter_t::run_wrapper, this, _fun)) { }
    void on_thread_switch() {
        rassert(get_thread_id().threadnum == get_num_threads() - 1);
        coro_t::spawn_sometime(run);
    }
private:
    void run_wrapper(const std::function<void()> &fun) {
        fun();
        tp->shutdown_thread_pool();
    }
};

// Runs the action 'fun()' on thread zero.
void run_in_thread_pool(const std::function<void()> &fun, int worker_threads) {
    linux_thread_pool_t thread_pool(worker_threads, false);
    starter_t starter(&thread_pool, fun);
    thread_pool.run_thread_pool(&starter);
}
