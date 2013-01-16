// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_IO_ARCH_HPP_
#define ARCH_IO_ARCH_HPP_

#include "arch/io/disk.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/timer.hpp"

/* Timer functions create (non-)periodic timers, callbacks for which are
 * executed on the same thread that they were created on. Thus, non-thread-safe
 * (but coroutine-safe) concurrency primitives can be used where appropriate.
 */
timer_token_t *add_timer(int64_t ms, timer_callback_t *callback);
timer_token_t *fire_timer_once(int64_t ms, timer_callback_t *callback);
void cancel_timer(timer_token_t *timer);

#endif /* ARCH_IO_ARCH_HPP_ */
