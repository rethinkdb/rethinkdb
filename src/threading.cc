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
    rassert(home_thread() == get_thread_id(), "home_thread(): %d, get_thread_id(): %d", home_thread().threadnum, get_thread_id().threadnum);
}
#endif

home_thread_mixin_t::home_thread_mixin_t(threadnum_t specified_home_thread)
    : real_home_thread(specified_home_thread) {
    assert_good_thread_id(specified_home_thread);
}
home_thread_mixin_t::home_thread_mixin_t()
    : real_home_thread(get_thread_id()) {
}


on_thread_t::on_thread_t(threadnum_t thread) {
    coro_t::move_to_thread(thread);
}
on_thread_t::~on_thread_t() {
    coro_t::move_to_thread(home_thread());
}


// The last thread is used as a utility thread, and is the launching point for the
// server.  This ensures that various system-level tasks are homed on the utility
// thread, and will not impact the operation of the data-hosting threads.
int get_num_db_threads() {
    return get_num_threads() - 1;
}
