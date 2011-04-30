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

// TODO: continue_on_thread() and call_later_on_this_thread are mostly obsolete because of
// coroutine-based thread switching.

// continue_on_thread() is used to send a message to another thread. If the 'thread' parameter is the
// thread that we are already on, then it returns 'true'; otherwise, it will cause the other
// thread's event loop to call msg->on_thread_switch().

inline bool continue_on_thread(int thread, linux_thread_message_t *msg) {
    assert_good_thread_id(thread);
    if (thread == linux_thread_pool_t::thread_id) {
        // The thread to continue on is the thread we are already on
        return true;
    } else {
        linux_thread_pool_t::thread->message_hub.store_message(thread, msg);
        return false;
    }
}

// call_later_on_this_thread() will cause msg->on_thread_switch() to be called from the main event loop
// of the thread we are currently on. It's a bit of a hack.

inline void call_later_on_this_thread(linux_thread_message_t *msg) {
    linux_thread_pool_t::thread->message_hub.store_message(linux_thread_pool_t::thread_id, msg);
}

/* TODO: It is common in the codebase right now to have code like this:

if (continue_on_thread(thread, msg)) call_later_on_this_thread(msg);

This is because originally clients would just call store_message() directly.
When continue_on_thread() was written, the code still assumed that the message's
callback would not be called before continue_on_thread() returned. Using
call_later_on_this_thread() is not ideal because it would be better to just
continue processing immediately if we are already on the correct thread, but
at the time it didn't seem worth rewriting it, so call_later_on_this_thread()
was added to make it easy to simulate the old semantics. */

#endif /* __ARCH_CORE_HPP__ */
