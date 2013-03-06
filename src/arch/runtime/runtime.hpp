// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_RUNTIME_HPP_
#define ARCH_RUNTIME_RUNTIME_HPP_

#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/coroutines.hpp"

typedef linux_thread_message_t thread_message_t;

int get_thread_id();

int get_num_threads();

#ifndef NDEBUG
void assert_good_thread_id(int thread);
#else
inline void assert_good_thread_id(UNUSED int thread) { }
#endif

// TODO: continue_on_thread() and call_later_on_this_thread are mostly obsolete because of
// coroutine-based thread switching.

// continue_on_thread() is used to send a message to another thread. If the 'thread' parameter is the
// thread that we are already on, then it returns 'true'; otherwise, it will cause the other
// thread's event loop to call msg->on_thread_switch().

bool continue_on_thread(int thread, linux_thread_message_t *msg);

// call_later_on_this_thread() will cause msg->on_thread_switch() to be called from the main event loop
// of the thread we are currently on. It's a bit of a hack.

void call_later_on_this_thread(linux_thread_message_t *msg);

/* TODO: It is common in the codebase right now to have code like this:

if (continue_on_thread(thread, msg)) call_later_on_this_thread(msg);

This is because originally clients would just call store_message() directly.
When continue_on_thread() was written, the code still assumed that the message's
callback would not be called before continue_on_thread() returned. Using
call_later_on_this_thread() is not ideal because it would be better to just
continue processing immediately if we are already on the correct thread, but
at the time it didn't seem worth rewriting it, so call_later_on_this_thread()
was added to make it easy to simulate the old semantics. */


#endif /* ARCH_RUNTIME_RUNTIME_HPP_ */
