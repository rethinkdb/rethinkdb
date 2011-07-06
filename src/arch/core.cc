#include "arch/core.hpp"

#include "utils2.hpp"
#include "arch/linux/coroutines.hpp"
#include "arch/linux/thread_pool.hpp"

int get_thread_id() {
    return linux_thread_pool_t::thread_id;
}

int get_num_threads() {
    return linux_thread_pool_t::thread_pool->n_threads;
}

#ifndef NDEBUG
void assert_good_thread_id(int thread) {
    rassert(thread >= 0, "(thread = %d)", thread);
    rassert(thread < get_num_threads(), "(thread = %d, n_threads = %d)", thread, get_num_threads());
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
