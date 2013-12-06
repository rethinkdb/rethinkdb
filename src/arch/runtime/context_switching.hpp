// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_
#define ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_

#include <pthread.h>

#include "errors.hpp"

#include "arch/io/concurrency.hpp"


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
    friend void context_switch(artificial_stack_context_ref_t *, artificial_stack_context_ref_t *);

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
    bool address_in_stack(void *);

    /* Returns `true` if the given address is in the stack's protection page. */
    bool address_is_stack_overflow(void *);

    /* Returns the base of the stack */
    void* get_stack_base() { return static_cast<char*>(stack) + stack_size; }

    /* Returns the end of the stack */
    void* get_stack_bound() { return stack; }

private:
    void *stack;
    size_t stack_size;
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
    threaded_context_ref_t() : lock(NULL), do_rethread(false) { store_virtual_thread(); }
    ~threaded_context_ref_t() { if (lock != NULL) delete lock; }

    // Every context reference has a condition that can be used to wake it up.
    // While it is active, it also holds a lock on the virtual_thread_mutexes of
    // its current virtual thread.
    system_cond_t cond;
    system_mutex_t::lock_t *lock;

    /* This is a fake */
    bool is_nil() { return false; }

    void wait();
    void rethread_to_current();
private:
    friend class threaded_stack_t;
    void restore_virtual_thread();
    void store_virtual_thread();

    bool do_rethread;
    linux_thread_pool_t *my_thread_pool;
    int32_t my_thread_id;
    linux_thread_t *my_thread;
};

void context_switch(threaded_context_ref_t *current_context, threaded_context_ref_t *dest_context);

class threaded_stack_t {
public:

    threaded_stack_t(void (*initial_fun_)(void), size_t stack_size);
    ~threaded_stack_t();

    threaded_context_ref_t context;

    /* These are fakes */
    bool address_in_stack(void *) { return true; }
    bool address_is_stack_overflow(void *) { return false; }
    void* get_stack_base() { guarantee(false); return NULL; }
    void* get_stack_bound() { guarantee(false); return NULL; }

private:
    static void *internal_run(void *p);

    pthread_t thread;
    void (*initial_fun)(void);
    system_cond_t launch_cond;

    // To fake the memory overhead of an artificial_stack_t, we actually
    // allocate one here and just let it sit around.
    // TODO: If we at some point want to use threaded_stack_t for production
    // stuff, we will have to remove this.
    artificial_stack_t dummy_stack;
};

/* ^^^^ Threaded version of context_switching ^^^^ */

#ifdef THREADED_COROUTINES
typedef threaded_stack_t coro_stack_t;
typedef threaded_context_ref_t coro_context_ref_t;
#else
typedef artificial_stack_t coro_stack_t;
typedef artificial_stack_context_ref_t coro_context_ref_t;
#endif


#endif /* ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_ */
