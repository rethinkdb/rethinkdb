#include "arch/linux/arch.hpp"
#include "arch/linux/coroutines.hpp"
#include "concurrency/cond_var.hpp"
#include "arch/linux/thread_pool.hpp"
#include "config/args.hpp"
#include <stdio.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <arch/arch.hpp>

#include "utils.hpp"

#ifdef VALGRIND
#include <valgrind/valgrind.h>
#endif

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#endif

perfmon_counter_t pm_active_coroutines("active_coroutines"),
                  pm_allocated_coroutines("allocated_coroutines");

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* We have a custom implementation of swapcontext() that doesn't swap the floating-point
registers, the SSE registers, or the signal mask. This is for performance reasons. */

extern "C" {
    typedef void *lw_ucontext_t; /* A stack pointer into a stack that has all the other context registers. */

    void lightweight_makecontext(lw_ucontext_t *ucp, void (*func) (void), void *stack, size_t stack_size) {
        uint64_t *sp; /* A pointer into the stack. */

        /* Start at the beginning. */
        sp = (uint64_t *) ((uintptr_t) stack + stack_size);

        /* Align stack. TODO: Is this necessary? */
        sp = (uint64_t *) (((uintptr_t) sp) & -16L);

        /* TODO: Figure out why some things (e.g. stats) break without this. */
        sp--;

        /* Set up the instruction pointer; this will be popped off the stack by
         * ret in swapcontext once all the other registers have been "restored". */
        sp--;
        *sp = (uint64_t) func;

        /* These registers (r12, r13, r14, r15, rbx, rbp) are going to be
         * popped off the stack by swapcontext; they're callee-saved, so
         * whatever happens to be in them will be ignored. */
        sp -= 6;

        /* Set up stack pointer. */
        *ucp = sp;

        /* Our coroutines never return, so we don't put anything else on the
         * stack. */
    }

    /* The register definitions and the definition of the lightweight context functions are
    derived from GLibC, which is covered under the LGPL. */

    extern void lightweight_swapcontext(lw_ucontext_t *oucp, const lw_ucontext_t uc);
    asm(
        ".globl lightweight_swapcontext\n"
        "lightweight_swapcontext:\n"
            /* Save preserved registers (the return address is already on the stack). */
            "pushq %r12\n"
            "pushq %r13\n"
            "pushq %r14\n"
            "pushq %r15\n"
            "pushq %rbx\n"
            "pushq %rbp\n"

            /* Save old stack pointer. */
            "movq %rsp, (%rdi)\n"

            /* Load the new stack pointer and the preserved registers. */
            "movq %rsi, %rsp\n"

            "popq %rbp\n"
            "popq %rbx\n"
            "popq %r15\n"
            "popq %r14\n"
            "popq %r13\n"
            "popq %r12\n"

            /* The following ret should return to the address set with
             * makecontext or with the previous swapcontext. The instruction
             * pointer is saved on the stack from the previous call (or
             * initialized with makecontext). */
            "ret\n"
    );
}

/* coro_context_t is only used internally within the coroutine logic. For performance reasons,
we recycle stacks and ucontexts; the coro_context_t represents a stack and a ucontext. */

struct coro_context_t : public intrusive_list_node_t<coro_context_t> {
    coro_context_t();

    /* The run() function is at the bottom of every coro_context_t's call stack. It repeatedly
    waits for a coroutine to run and then calls that coroutine's run() method. */
    static void run();

    ~coro_context_t();

    /* The guts of the coro_context_t */
    void *stack;
    lw_ucontext_t env; // A pointer into the stack.
#ifdef VALGRIND
    unsigned int valgrind_stack_id;
#endif
};

/* The coroutine we're currently in, if any. NULL if we are in the main context. */
static __thread coro_t *current_coro = NULL;

/* The main context. */
static __thread lw_ucontext_t scheduler;

/* The previous context. */
static __thread coro_t *prev_coro = NULL;

/* A list of coro_context_t objects that are not in use. */
static __thread intrusive_list_t<coro_context_t> *free_contexts = NULL;

/* An array of ints counting the number of coros on each thread */
static __thread int coro_context_count = 0;

/* coro_globals_t */

coro_globals_t::coro_globals_t() {
    rassert(!current_coro);

    rassert(free_contexts == NULL);
    free_contexts = new intrusive_list_t<coro_context_t>;
}

coro_globals_t::~coro_globals_t() {
    rassert(!current_coro);

    /* Destroy remaining coroutines */
    while (coro_context_t *s = free_contexts->head()) {
        free_contexts->remove(s);
        delete s;
    }

    delete free_contexts;
    free_contexts = NULL;
}

/* coro_context_t */

coro_context_t::coro_context_t() {
    pm_allocated_coroutines++;
    coro_context_count++;
    guarantee(coro_context_count < MAX_COROS_PER_THREAD, "Too many coroutines "
            "allocated on this thread. This is problem due to a misuse of the "
            "coroutines\n");

    stack = malloc_aligned(coro_stack_size, getpagesize());

    /* Protect the end of the stack so that we crash when we get a stack overflow instead of
    corrupting memory. */
    mprotect(stack, getpagesize(), PROT_NONE);

    /* Register our stack with Valgrind so that it understands what's going on and doesn't
    create spurious errors */
#ifdef VALGRIND
    valgrind_stack_id = VALGRIND_STACK_REGISTER(stack, (intptr_t)stack + coro_stack_size);
#endif

    /* run() is the main worker loop for a coroutine. */
    lightweight_makecontext(&env, &coro_context_t::run, stack, coro_stack_size);
}

void coro_context_t::run() {
    coro_context_t *self = current_coro->context;

    /* Make sure we're on the right stack. */
#ifndef NDEBUG
    char dummy;
    rassert(&dummy >= (char*)self->stack);
    rassert(&dummy < (char*)self->stack + coro_stack_size);
#endif

    while (true) {
        current_coro->run();

        if (prev_coro) {
            lightweight_swapcontext(&self->env, prev_coro->context->env);
        } else {
            lightweight_swapcontext(&self->env, scheduler);
        }
    }
}

coro_context_t::~coro_context_t() {
#ifdef VALGRIND
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif

    mprotect(stack, getpagesize(), PROT_READ|PROT_WRITE); //Undo protections changes

    free(stack);

    pm_allocated_coroutines--;
    coro_context_count--;
}

/* coro_t */


coro_t::coro_t(const boost::function<void()>& deed, int thread) :
    deed_(deed),
    current_thread_(thread),
    original_free_contexts_thread_(linux_thread_pool_t::thread_id),
    notified_(false),
    waiting_(true)
{
    assert_good_thread_id(thread);

    pm_active_coroutines++;

    /* Find us a stack */
    if (free_contexts->size() == 0) {
        context = new coro_context_t();
    } else {
        context = free_contexts->tail();
        free_contexts->remove(context);
    }
}

void return_context_to_free_contexts(coro_context_t *context) {
    free_contexts->push_back(context);
}

coro_t::~coro_t() {
    /* Return the context to the free-contexts list we took it from. */
    do_on_thread(original_free_contexts_thread_, boost::bind(return_context_to_free_contexts, context));

    pm_active_coroutines--;
}

void coro_t::run() {
    rassert(current_coro == this);
    waiting_ = false;

    deed_();

    delete this;
}

coro_t *coro_t::self() {   /* class method */
    return current_coro;
}

void coro_t::wait() {   /* class method */
    rassert(self(), "Not in a coroutine context");

#ifndef NDEBUG
    /* It's not safe to wait() in a catch clause of an exception handler. We use the non-standard
    GCC-only interface "cxxabi.h" to figure out if we're in the catch clause of an exception
    handler. In C++0x we will be able to use std::current_exception() instead. */
    rassert(!abi::__cxa_current_exception_type());
#endif

    rassert(!current_coro->waiting_);
    current_coro->waiting_ = true;

    rassert(current_coro);
    if (prev_coro) {
        lightweight_swapcontext(&current_coro->context->env, prev_coro->context->env);
    } else {
        lightweight_swapcontext(&current_coro->context->env, scheduler);
    }

    rassert(current_coro);

    rassert(current_coro->waiting_);
    current_coro->waiting_ = false;
}

void coro_t::yield() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify();
    self()->wait();
}

void coro_t::notify_now() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_ == linux_thread_pool_t::thread_id);

#ifndef NDEBUG
    /* It's not safe to notify_now() in the catch-clause of an exception handler for the same
    reason we can't wait(). */
    rassert(!abi::__cxa_current_exception_type());
#endif

    coro_t *prev_prev_coro = prev_coro;
    prev_coro = current_coro;
    current_coro = this;

    if (prev_coro) {
        lightweight_swapcontext(&prev_coro->context->env, context->env);
    } else {
        lightweight_swapcontext(&scheduler, context->env);
    }

    rassert(current_coro == this);
    current_coro = prev_coro;
    prev_coro = prev_prev_coro;
}

void coro_t::notify() {
    rassert(!notified_);
    notified_ = true;

    /* notify() doesn't switch to the coroutine immediately; instead, it just pushes
    the coroutine onto the event queue. */

    /* current_thread is the thread that the coroutine lives on, which may or may not be the
    same as get_thread_id(). */
    linux_thread_pool_t::thread->message_hub.store_message(current_thread_, this);
}

void coro_t::move_to_thread(int thread) {   /* class method */
    assert_good_thread_id(thread);
    if (thread == self()->current_thread_) {
        // If we're trying to switch to the thread we're currently on, do nothing.
        return;
    }
    self()->current_thread_ = thread;
    self()->notify();
    wait();
}

void coro_t::on_thread_switch() {
    rassert(notified_);
    notified_ = false;

    notify_now();
}

void coro_t::set_coroutine_stack_size(size_t size) {
    coro_stack_size = size;
}

/* Called by SIGSEGV handler to identify segfaults that come from overflowing a coroutine's
stack. Could also in theory be used by a function to check if it's about to overflow
the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    void *base = (void *)floor_aligned((intptr_t)addr, getpagesize());
    return current_coro && current_coro->context->stack == base;
}

void coro_t::spawn(const boost::function<void()>& deed) {
    spawn_on_thread(linux_thread_pool_t::thread_id, deed);
}

void coro_t::spawn_now(const boost::function<void()> &deed) {
    (new coro_t(deed, linux_thread_pool_t::thread_id))->notify_now();
}

void coro_t::spawn_on_thread(int thread, const boost::function<void()>& deed) {
    (new coro_t(deed, thread))->notify();
}

