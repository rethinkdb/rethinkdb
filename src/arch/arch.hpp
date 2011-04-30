#ifndef __ARCH_ARCH_HPP__
#define __ARCH_ARCH_HPP__

/* Select platform-specific stuff */

#include "arch/core.hpp"

/* #if WE_ARE_ON_LINUX */

#include "arch/linux/arch.hpp"
typedef linux_io_config_t platform_io_config_t;

/* #elif WE_ARE_ON_WINDOWS

#include "arch/win32/arch.hpp"
typedef win32_io_config_t platform_io_config_t

#elif ...

...

#endif */

/* Optionally mock the IO layer */

#ifndef MOCK_IO_LAYER

typedef platform_io_config_t io_config_t;

#else

#include "arch/mock/arch.hpp"
typedef mock_io_config_t<platform_io_config_t> io_config_t;

#endif

/* Move stuff into global namespace */

typedef io_config_t::thread_pool_t thread_pool_t;

typedef io_config_t::file_t file_t;
typedef io_config_t::direct_file_t direct_file_t;
typedef io_config_t::nondirect_file_t nondirect_file_t;
typedef io_config_t::iocallback_t iocallback_t;

typedef io_config_t::tcp_listener_t tcp_listener_t;
typedef io_config_t::tcp_listener_callback_t tcp_listener_callback_t;

typedef io_config_t::tcp_conn_t tcp_conn_t;

// continue_on_thread() is used to send a message to another thread. If the 'thread' parameter is the
// thread that we are already on, then it returns 'true'; otherwise, it will cause the other
// thread's event loop to call msg->on_thread_switch().
inline bool continue_on_thread(int thread, thread_message_t *msg) {
    assert_good_thread_id(thread);
    return io_config_t::continue_on_thread(thread, msg);
}

// call_later_on_this_thread() will cause msg->on_thread_switch() to be called from the main event loop
// of the thread we are currently on. It's a bit of a hack.
//
/* TODO: It is common in the codebase right now to have code like this:

if (continue_on_thread(thread, msg)) call_later_on_this_thread(msg);

This is because originally clients would just call store_message() directly.
When continue_on_thread() was written, the code still assumed that the message's
callback would not be called before continue_on_thread() returned. Using
call_later_on_this_thread() is not ideal because it would be better to just
continue processing immediately if we are already on the correct thread, but
at the time it didn't seem worth rewriting it, so call_later_on_this_thread()
was added to make it easy to simulate the old semantics. */

inline void call_later_on_this_thread(thread_message_t *msg) {
    return io_config_t::call_later_on_this_thread(msg);
}


/* Timer functions create (non-)periodic timers, callbacks for which are
 * executed on the same thread that they were created on. Thus, non-thread-safe
 * (but coroutine-safe) concurrency primitives can be used where appropriate.
 */
inline timer_token_t *add_timer(long ms, void (*callback)(void *), void *ctx) {
    return io_config_t::add_timer(ms, callback, ctx);
}
inline timer_token_t *fire_timer_once(long ms, void (*callback)(void *), void *ctx) {
    return io_config_t::fire_timer_once(ms, callback, ctx);
}
inline void cancel_timer(timer_token_t *timer) {
    io_config_t::cancel_timer(timer);
}

void co_read(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account);
void co_write(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account);

#include "arch/spinlock.hpp"
#include "arch/timing.hpp"

#endif /* __ARCH_ARCH_HPP__ */
