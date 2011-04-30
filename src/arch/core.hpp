#ifndef __ARCH_CORE_HPP__
#define __ARCH_CORE_HPP__

/* HACK. See Tim for more information. */

#include "arch/linux/coroutines.hpp"
#include "arch/linux/thread_pool.hpp"

typedef linux_thread_message_t thread_message_t;

inline int get_thread_id() {
    return linux_thread_pool_t::thread_id;
}

inline int get_num_threads() {
    return linux_thread_pool_t::thread_pool->n_threads;
}

#ifndef NDEBUG
inline void assert_good_thread_id(int thread) {
    rassert(thread >= 0, "(thread = %d)", thread);
    rassert(thread < get_num_threads(), "(thread = %d, n_threads = %d)", thread, get_num_threads());
}
#else
#define assert_good_thread_id(thread) do { } while(0)
#endif

#endif /* __ARCH_CORE_HPP__ */
