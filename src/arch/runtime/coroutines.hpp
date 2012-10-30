// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_COROUTINES_HPP_
#define ARCH_RUNTIME_COROUTINES_HPP_

#ifndef NDEBUG
#include <string>
#endif

#include "arch/runtime/runtime_utils.hpp"
#include "arch/runtime/context_switching.hpp"
#include "utils.hpp"

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

int get_thread_id();
struct coro_globals_t;

/* A coro_t represents a fiber of execution within a thread. Create one with spawn_*(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify_*() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

class coro_t : private linux_thread_message_t, public intrusive_list_node_t<coro_t>, public home_thread_mixin_t {
public:
    friend bool is_coroutine_stack_overflow(void *);

    template<class Callable>
    static void spawn_now_dangerously(const Callable &action) {
        get_and_init_coro(action)->notify_now_deprecated();
    }

    template<class Callable>
    static void spawn_sometime(const Callable &action) {
        get_and_init_coro(action)->notify_sometime();
    }

    // TODO: spawn_later_ordered is usually what naive people want,
    // but it's such a long and onerous name.  It should have the
    // shortest name.
    template<class Callable>
    static void spawn_later_ordered(const Callable &action) {
        get_and_init_coro(action)->notify_later_ordered();
    }

    // Use coro_t::spawn_*(boost::bind(...)) for spawning with parameters.

    /* `spawn()` and `notify()` are aliases for `spawn_later_ordered()` and
    `notify_later_ordered()`. They are deprecated and new code should not use
    them. */
    template<class Callable>
    static void spawn(const Callable &action) {
        spawn_later_ordered(action);
    }

    /* Pauses the current coroutine until it is notified */
    static void wait();

    /* Gives another coroutine a chance to run, but schedules this coroutine to
    be run at some point in the future. Might not preserve order; two calls to
    `yield()` by different coroutines may return in a different order than they
    began in. */
    static void yield();

    /* Returns a pointer to the current coroutine, or `NULL` if we are not in a
    coroutine. */
    static coro_t *self();

    /* Transfers control immediately to the coroutine. Returns when the
    coroutine calls `wait()`.

    Note: `notify_now()` may become deprecated eventually. The original purpose
    was to provide better performance than could be achieved with
    `notify_later_ordered()`, but `notify_sometime()` is now filling that role
    instead. */
    void notify_now_deprecated();

    /* Schedules the coroutine to be woken up eventually. Can be safely called
    from any thread. Returns immediately. Does not provide any ordering
    guarantees. If you don't need the ordering guarantees that
    `notify_later_ordered()` provides, use `notify_sometime()` instead. */
    void notify_sometime();

    // TODO: notify_later_ordered is usually what naive people want
    // and should get, but it's such a long and onerous name.  It
    // should have the shortest name.

    /* Pushes the coroutine onto the event queue for the thread it's currently
    on, such that it will be run. This can safely be called from any thread.
    Returns immediately. If you call `notify_later_ordered()` on two coroutines
    that are on the same thread, they will run in the same order you call
    `notify_later_ordered()` in. */
    void notify_later_ordered();

#ifndef NDEBUG
    // A unique identifier for this particular instance of coro_t over
    // the lifetime of the process.
    static int64_t selfname() {
        coro_t *self = coro_t::self();
        return self ? self->selfname_number : 0;
    }

    const std::string& get_coroutine_type() { return coroutine_type; }
#endif

    static void set_coroutine_stack_size(size_t size);

    artificial_stack_t * get_stack();

private:
    /* When called from within a coroutine, schedules the coroutine to be run on
    the given thread and then suspends the coroutine until that other thread
    picks it up again. Do not call this directly; use `on_thread_t` instead. */
    friend class on_thread_t;
    static void move_to_thread(int thread);

    // Contructor sets up the stack, get_and_init_coro will load a function to be run
    //  at which point the coroutine can be notified
    coro_t();

    // If this function footprint ever changes, you may need to update the parse_coroutine_info function
    template<class Callable>
    static coro_t * get_and_init_coro(const Callable &action) {
        coro_t *coro = get_coro();
#ifndef NDEBUG
        coro->parse_coroutine_type(__PRETTY_FUNCTION__);
#endif
        coro->action_wrapper.reset(action);
        return coro;
    }

    static coro_t * get_coro();

    static void return_coro_to_free_list(coro_t *coro);

    static void run() NORETURN;

    friend struct coro_globals_t;
    ~coro_t();

    virtual void on_thread_switch();

    artificial_stack_t stack;

    int current_thread_;

    // Sanity check variables
    bool notified_;
    bool waiting_;

    callable_action_wrapper_t action_wrapper;

#ifndef NDEBUG
    int64_t selfname_number;
    std::string coroutine_type;
    void parse_coroutine_type(const char *coroutine_function);
#endif

    DISABLE_COPYING(coro_t);
};

/* Returns true if the given address is in the protection page of the current coroutine. */
bool is_coroutine_stack_overflow(void *addr);
bool coroutines_have_been_initialized();

class home_coro_mixin_t {
private:
    coro_t *home_coro;
public:
    home_coro_mixin_t();
    void assert_coro();
};

#endif // ARCH_RUNTIME_COROUTINES_HPP_
