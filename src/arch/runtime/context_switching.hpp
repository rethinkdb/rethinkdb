// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_
#define ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_

#include "errors.hpp"

/* ____ Threaded version of context_switching ____ */

#include <pthread.h>
#include "arch/io/concurrency.hpp"

class linux_thread_pool_t;
class linux_thread_t;

class context_ref_t {
public:
    context_ref_t() : lock(NULL), do_rethread(false) { store_virtual_thread(); }
    ~context_ref_t() { if (lock != NULL) delete lock; }
    system_cond_t cond;
    system_mutex_t::lock_t *lock;

    /* This is a fake */
    bool is_nil() { return false; }

    void wait();
    void restore_virtual_thread();
    void store_virtual_thread();
    void rethread_to_current(system_cond_t *current_cond);
private:
    bool do_rethread;
    system_cond_t *on_rethreaded_cond;
    linux_thread_pool_t *my_thread_pool;
    int32_t my_thread_id;
    linux_thread_t *my_thread;
};

void context_switch(context_ref_t *current_cond, context_ref_t *dest_cond);

class threaded_context_t {
public:

    threaded_context_t(void (*initial_fun_)(void), size_t);
    ~threaded_context_t();

    context_ref_t context;

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
};
typedef threaded_context_t artificial_stack_t;

/* ^^^^ Threaded version of context_switching ^^^^ */

#if 0

/* Note that `context_ref_t` is not a POD type. We could make it a POD type, but
at the cost of removing some safety guarantees provided by the constructor and
destructor. */

struct context_ref_t {

    /* Creates a nil `context_ref_t`. */
    context_ref_t();

    /* Destroys a `context_ref_t`. It must be nil at the time that this
    destructor is called; if you're trying to call the destructor and it's
    non-nil, that means you're leaking a stack somewhere. */
    ~context_ref_t();

    /* Returns `true` if the `context_ref_t` is nil, and false otherwise. */
    bool is_nil();

private:
    friend class artificial_stack_t;
    friend void context_switch(context_ref_t *, context_ref_t *);

    /* `pointer` points to a location on the stack of the context in question.
    From that pointer, we find the instruction pointer, relevant registers, and
    so on. */
    void *pointer;

    DISABLE_COPYING(context_ref_t);
};

class artificial_stack_t {
public:

    /* `artificial_stack_t()` sets up an artificial context. Once it is set up,
    you can use `context` to swap into and out of it. When you call
    `~artificial_stack_t()`, the original context must have been returned to
    `context` again. */
    artificial_stack_t(void (*initial_fun)(void), size_t stack_size);
    ~artificial_stack_t();

    context_ref_t context;

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
    context_ref_t *current_context_out,
    context_ref_t *dest_context_in);

#endif

#endif /* ARCH_RUNTIME_CONTEXT_SWITCHING_HPP_ */
