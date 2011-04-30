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
