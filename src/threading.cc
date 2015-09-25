#include "threading.hpp"

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "errors.hpp"

#ifndef NDEBUG
void home_thread_mixin_debug_only_t::assert_thread() const {
    rassert(get_thread_id() == real_home_thread);
}
#endif

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t(DEBUG_VAR threadnum_t specified_home_thread)
#ifndef NDEBUG
    : real_home_thread(specified_home_thread)
#endif
{ }

home_thread_mixin_debug_only_t::home_thread_mixin_debug_only_t()
#ifndef NDEBUG
    : real_home_thread(get_thread_id())
#endif
{ }

#ifndef NDEBUG
void home_thread_mixin_t::assert_thread() const {
    rassert(home_thread() == get_thread_id());
}
#endif

home_thread_mixin_t::home_thread_mixin_t(threadnum_t specified_home_thread)
    : real_home_thread(specified_home_thread) {
    assert_good_thread_id(specified_home_thread);
}
home_thread_mixin_t::home_thread_mixin_t()
    : real_home_thread(get_thread_id()) {
    assert_good_thread_id(real_home_thread); // TODO ATN
}


on_thread_t::on_thread_t(threadnum_t thread) {
    coro_t::move_to_thread(thread);
}
on_thread_t::~on_thread_t() {
    coro_t::move_to_thread(home_thread());
}


// The last thread is a service thread that runs an connection acceptor, a log
// writer, and possibly similar services, and does not run any db code (caches,
// serializers, etc). The reasoning is that when the acceptor (and possibly other
// utils) get placed on an event queue with the db code, the latency for these utils
// can increase significantly. In particular, it causes timeout bugs in clients that
// expect the acceptor to work faster.

// TODO: ^^ This comment is way outdated, and this function (and the practice of
// adding 1 to the thread count) is, too.
int get_num_db_threads() {
    return get_num_threads() - 1;
}
