// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_
#define ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_

#ifdef _WIN32

struct fiber_context_ref_t {
	void* fiber = nullptr;
	bool is_nil() { return fiber == nullptr; }
	~fiber_context_ref_t();
};

void coro_initialize_for_thread();

class fiber_stack_t {
public:
    fiber_stack_t(void(*initial_fun)(void), size_t stack_size);
    ~fiber_stack_t();
    fiber_context_ref_t context;
    bool has_stack_info;
    char *stack_base;
    char *stack_limit;
    bool address_in_stack(const void *addr) const;
    bool address_is_stack_overflow(const void *addr) const;
    size_t free_space_below(const void *addr) const;

    /* These two are currently not implemented for fiber stacks.
    I think fibers always have some overflow protection though? */
    void enable_overflow_protection() {}
    void disable_overflow_protection() {}
};

void context_switch(fiber_context_ref_t *current_context_out, fiber_context_ref_t *dest_context_in);

typedef fiber_stack_t coro_stack_t;
typedef fiber_context_ref_t coro_context_ref_t;

#else

#include <pthread.h>

#include "errors.hpp"

#include "arch/io/concurrency.hpp"
#include "containers/scoped.hpp"


/* Note that `artificial_stack_context_ref_t` is not a POD type. We could make it a POD type, but
at the cost of removing some safety guarantees provided by the constructor and
destructor. */

struct artificial_stack_context_ref_t {

    /* Creates a nil `context_ref_t`. */
    artificial_stack_context_ref_t();

    /* Destroys a `context_ref_t`. It must be nil at the time that this
    destructor is called; if you're trying to call the destructor and it's
    non-nil, that means you're leaking a stack somewhere. */
    ~artificial_stack_context_ref_t();

    /* Returns `true` if the `artificial_stack_context_ref_t` is nil, and false otherwise. */
    bool is_nil();

private:
    friend class artificial_stack_t;
    friend void context_switch(artificial_stack_context_ref_t *,
                               artificial_stack_context_ref_t *);

    /* `pointer` points to a location on the stack of the context in question.
    From that pointer, we find the instruction pointer, relevant registers, and
    so on. */
    void *pointer;

    DISABLE_COPYING(artificial_stack_context_ref_t);
};

class artificial_stack_t {
public:

    /* `artificial_stack_t()` sets up an artificial context. Once it is set up,
    you can use `context` to swap into and out of it. When you call
    `~artificial_stack_t()`, the original context must have been returned to
    `context` again. */
    artificial_stack_t(void (*initial_fun)(void), size_t stack_size);
    ~artificial_stack_t();

    artificial_stack_context_ref_t context;

    /* Returns `true` if the given address is on this stack or in its protection
    page. */
    bool address_in_stack(const void *addr) const;

    /* Returns `true` if the given address is in the stack's protection page. */
    bool address_is_stack_overflow(const void *addr) const;

    /* Returns the base of the stack */
    void *get_stack_base() const { return stack.get() + stack_size; }

    /* Returns the end of the stack */
    void *get_stack_bound() const { return stack.get(); }

    /* Returns how many more bytes below the given address can be used */
    size_t free_space_below(const void *addr) const;

    /* Enables stack-smashing protection for this stack, if not already enabled */
    void enable_overflow_protection();
    /* Disables stack-smashing protection for this stack, if currently enabled */
    void disable_overflow_protection();

private:
    scoped_page_aligned_ptr_t<char> stack;
    size_t stack_size;
    bool overflow_protection_enabled;
#ifdef VALGRIND
    int valgrind_stack_id;
#endif
};

/* `context_switch()` switches from one context to another.
`current_context_out` must be nil; it will be filled with the context that we
were in when we called `context_switch()`. `destination_context_in` must be
non-nil; it will be set to nil, and we will switch to it. */
void context_switch(
    artificial_stack_context_ref_t *current_context_out,
    artificial_stack_context_ref_t *dest_context_in);



/* ____ Threaded version of context_switching ____ */

/*
 * This is an alternative implementation where each context runs in a thread of
 * its own. The primary use case for this is to allow certain profiling tools
 * that would otherwise get confused by our artificial_stack switches to be used
 * with RethinkDB. Another use case is to ease porting RethinkDB to new
 * architectures in cases where performance doesn't matter much.
 *
 * Internally, one thread is created per threaded_stack_t. To be compatible
 * with the artificial_stack_t implementation, we simulate a fixed number of
 * "virtual threads" (up to MAX_THREADS) Each threaded_stack_t has an associated
 * virtual thread and only one threaded_stack_t can be active per virtual thread
 * at any time, which is enforced through a mutex per virtual thread and a
 * condition in each threaded_stack_t.
 *
 * A context switch then is simply notifying the condition on the threaded_stack_t
 * that should be swapped in and then making the previous context wait on its own
 * condition (there is some additional logic for handling virtual thread
 * switches and switching from/to the global context).
 */

class linux_thread_pool_t;
class linux_thread_t;

class threaded_context_ref_t {
public:
    threaded_context_ref_t() : do_rethread(false), do_shutdown(false) {
        store_virtual_thread();
    }

    /* Every context reference has a condition that can be used to wake it up.
    While it is active, it also holds a lock on the virtual_thread_mutexes of
    its current virtual thread. */
    system_cond_t cond;
    scoped_ptr_t<system_mutex_t::lock_t> lock;

    /* Returns `true` if the threaded_context_t has not been used yet,
    or if it is a reference to the scheduler context and the scheduler is currently
    running.
    (in other words: if it doesn't currently hold a context reference) */
    bool is_nil() { return !lock.has(); }

    void wait();
    void rethread_to_current();
private:
    friend class threaded_stack_t;
    void restore_virtual_thread();
    void store_virtual_thread();

    bool do_rethread;
    bool do_shutdown;

    /* These variables store the values that `restore_virtual_thread()` loads into
    linux_thread_pool_t's `thread_pool`, `thread_id` and `thread` variables.
    Those variables are thread-local in `linux_thread_pool_t`. Since a threaded
    context is actually run in a thread of its own, we have to initialize these
    values when the threaded context is activated.
    More specifically, we set them to the same values that they had when the
    `threaded_context_ref_t` was constructed, so it appears like it ran in
    the same thread. Unless the context is re-threaded, in which case
    we set them to the values taken from the thread that the context is re-threaded
    to. */
    linux_thread_pool_t *my_thread_pool;
    int32_t my_thread_id;
    linux_thread_t *my_thread;

    DISABLE_COPYING(threaded_context_ref_t);
};

void context_switch(threaded_context_ref_t *current_context,
                    threaded_context_ref_t *dest_context);

class threaded_stack_t {
public:

    threaded_stack_t(void (*initial_fun_)(void), size_t stack_size);
    ~threaded_stack_t();

    threaded_context_ref_t context;

    /* Returns `true` if the given address is on this stack or in its protection
    page. */
    bool address_in_stack(const void *addr) const;

    /* Returns `true` if the given address is in the stack's protection page. */
    bool address_is_stack_overflow(const void *addr) const;

    /* Returns the base of the stack */
    void *get_stack_base() const;

    /* Returns the end of the stack */
    void *get_stack_bound() const;

    /* Returns how many more bytes below the given address can be used */
    size_t free_space_below(const void *addr) const;

    /* These two are currently not implemented for threaded stacks. */
    void enable_overflow_protection() {}
    void disable_overflow_protection() {}

private:
    static void *internal_run(void *p);
    void get_stack_addr_size(void **stackaddr_out, size_t *stacksize_out) const;

    pthread_t thread;
    void (*initial_fun)(void);
    system_cond_t launch_cond;

    // To fake the memory overhead of an artificial_stack_t, we actually
    // allocate one here and just let it sit around.
    // TODO: If we at some point want to use threaded_stack_t for production
    // stuff, we will have to remove this.
    artificial_stack_t dummy_stack;

    DISABLE_COPYING(threaded_stack_t);
};

/* ^^^^ Threaded version of context_switching ^^^^ */

#ifdef THREADED_COROUTINES
typedef threaded_stack_t coro_stack_t;
typedef threaded_context_ref_t coro_context_ref_t;
#else
typedef artificial_stack_t coro_stack_t;
typedef artificial_stack_context_ref_t coro_context_ref_t;
#endif

#endif /* !defined(_WIN32) */
#endif /* ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_ */
