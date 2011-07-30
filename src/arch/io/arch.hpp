#ifndef __ARCH_IO_ARCH_HPP__
#define __ARCH_IO_ARCH_HPP__

#include "arch/io/disk.hpp"
#include "arch/io/network.hpp"
#include "arch/runtime/thread_pool.hpp"

/* Timer functions create (non-)periodic timers, callbacks for which are
 * executed on the same thread that they were created on. Thus, non-thread-safe
 * (but coroutine-safe) concurrency primitives can be used where appropriate.
 */
inline timer_token_t *add_timer(long ms, void (*callback)(void *), void *ctx) {
    return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, ctx, false);
}

inline timer_token_t *fire_timer_once(long ms, void (*callback)(void *), void *ctx) {
    return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, ctx, true);
}

inline void cancel_timer(timer_token_t *timer) {
    linux_thread_pool_t::thread->timer_handler.cancel_timer(timer);
}

inline long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

inline long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

#endif /* __ARCH_IO_ARCH_HPP__ */
