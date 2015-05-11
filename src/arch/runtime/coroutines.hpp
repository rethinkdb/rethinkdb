// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_COROUTINES_HPP_
#define ARCH_RUNTIME_COROUTINES_HPP_

#ifndef NDEBUG
#include <string>
#endif

#include "arch/compiler.hpp"
#include "arch/runtime/callable_action.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "threading.hpp"
#include "time.hpp"

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

// Enable cross-coroutine backtraces in debug mode, or when coro profiling is enabled
#if (!defined(NDEBUG) && !defined(__MACH__)) || defined(ENABLE_CORO_PROFILER)
#define CROSS_CORO_BACKTRACES            1
#endif
#define CROSS_CORO_BACKTRACES_MAX_SIZE  64

threadnum_t get_thread_id();
struct coro_globals_t;


struct coro_profiler_mixin_t {
#ifdef ENABLE_CORO_PROFILER
    coro_profiler_mixin_t() : last_resumed_at(0), last_sample_at(0) { }
    ticks_t last_resumed_at;
    ticks_t last_sample_at;
#endif
};


/* A coro_t represents a fiber of execution within a thread. Create one with spawn_*(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify_*() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

class coro_t : private coro_profiler_mixin_t,
               private linux_thread_message_t,
               public intrusive_list_node_t<coro_t>,
               public home_thread_mixin_t {
public:
    friend bool is_coroutine_stack_overflow(void *);

    template<class Callable>
    static void spawn_now_dangerously(Callable &&action) {
        coro_t *coro = get_and_init_coro(std::forward<Callable>(action));
        coro->notify_now_deprecated();
    }

    template<class Callable>
    static coro_t *spawn_sometime(Callable &&action) {
        coro_t *coro = get_and_init_coro(std::forward<Callable>(action));
        coro->notify_sometime();
        return coro;
    }

    /* Whenever possible, `spawn_sometime()` should be used instead of
    `spawn_later_ordered()` (or `spawn_ordered()`). `spawn_later_ordered()` does not
    honor scheduler priorities. */
    template<class Callable>
    static coro_t *spawn_later_ordered(Callable &&action) {
        coro_t *coro = get_and_init_coro(std::forward<Callable>(action));
        coro->notify_later_ordered();
        return coro;
    }

    template<class Callable>
    static void spawn_ordered(Callable &&action) {
        spawn_later_ordered(std::forward<Callable>(action));
    }

    // Use coro_t::spawn_*(std::bind(...)) for spawning with parameters.

    /* Pauses the current coroutine until it is notified */
    static void wait();

    /* Gives another coroutine a chance to run, but schedules this coroutine to
    be run at some point in the future. Might not preserve order; two calls to
    `yield()` by different coroutines may return in a different order than they
    began in. */
    static void yield();
    /* Like `yield()`, but guarantees that the ordering of coroutines calling 
    `yield_ordered()` is maintained. */
    static void yield_ordered();

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
    `notify_later_ordered()` provides, use `notify_sometime()`. */
    void notify_sometime();

    /* Pushes the coroutine onto the event queue for the thread it's currently
    on, such that it will be run. This can safely be called from any thread.
    Returns immediately. If you call `notify_later_ordered()` on two coroutines
    that are on the same thread, they will run in the same order you call
    `notify_later_ordered()` in. */
    // DEPRECATED:  Call notify_ordered, its name is shorter.
    void notify_later_ordered();

    void notify_ordered() {
        notify_later_ordered();
    }


#ifndef NDEBUG
    // A unique identifier for this particular instance of coro_t over
    // the lifetime of the process.
    static int64_t selfname() {
        coro_t *running_coro = self();
        return running_coro ? running_coro->selfname_number : 0;
    }

    const std::string& get_coroutine_type() { return coroutine_type; }
#endif

    static void set_coroutine_stack_size(size_t size);

    coro_stack_t *get_stack();

    void set_priority(int _priority) {
        linux_thread_message_t::set_priority(_priority);
    }
    int get_priority() const {
        return linux_thread_message_t::get_priority();
    }

    /* Copies the backtrace from the time of spawning the coroutine into
    `buffer_out`, which has to be allocated before calling the function.
    `size` must contain the maximum number of entries to store.
    Returns how many entries have been deposited into `buffer_out`. */
    int copy_spawn_backtrace(void **buffer_out, int size) const;

private:
    /* When called from within a coroutine, schedules the coroutine to be run on
    the given thread and then suspends the coroutine until that other thread
    picks it up again. Do not call this directly; use `on_thread_t` instead. */
    friend class on_thread_t;
    static void move_to_thread(threadnum_t thread);

    // Constructor sets up the stack, get_and_init_coro will load a function to be run
    //  at which point the coroutine can be notified
    coro_t();

    // Generates a spawn-time backtrace and stores it into `spawn_backtrace`.
    void grab_spawn_backtrace();

    // If this function footprint ever changes, you may need to update the parse_coroutine_info function
    template<class Callable>
    static coro_t *get_and_init_coro(Callable &&action) {
        coro_t *coro = get_coro();
#ifndef NDEBUG
        coro->parse_coroutine_type(CURRENT_FUNCTION_PRETTY);
#endif
        coro->grab_spawn_backtrace();
        coro->action_wrapper.reset(std::forward<Callable>(action));

        // If we were called from a coroutine, the new coroutine inherits our
        // caller's priority.
        if (self() != NULL) {
            coro->set_priority(self()->get_priority());
        } else {
            // Otherwise, just reset to the default.
            coro->set_priority(MESSAGE_SCHEDULER_DEFAULT_PRIORITY);
        }

        return coro;
    }

    static coro_t *get_coro();

    static void return_coro_to_free_list(coro_t *coro);
    static void maybe_evict_from_free_list();

    static void run() NORETURN;

    friend class coro_profiler_t;
    friend struct coro_globals_t;
    ~coro_t();

    virtual void on_thread_switch();

    coro_stack_t stack;

    threadnum_t current_thread_;

    // Sanity check variables
    bool notified_;
    bool waiting_;

    callable_action_wrapper_t action_wrapper;

#ifndef NDEBUG
    int64_t selfname_number;
    std::string coroutine_type;
    void parse_coroutine_type(const char *coroutine_function);
#endif

#ifdef CROSS_CORO_BACKTRACES
    int spawn_backtrace_size;
    void *spawn_backtrace[CROSS_CORO_BACKTRACES_MAX_SIZE];
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
