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

callable_action_wrapper_t::callable_action_wrapper_t() :
    action_on_heap(false),
    action_(NULL)
{ }

callable_action_wrapper_t::~callable_action_wrapper_t()
{
    if(action_ != NULL) {
        reset();
    }
}

void callable_action_wrapper_t::reset() {
    rassert(action_ != NULL);

    if (action_on_heap) {
        delete action_;
        action_ = NULL;
        action_on_heap = false;
    } else {
        action_->~callable_action_t();
        action_ = NULL;
    }
}

void callable_action_wrapper_t::run() {
    rassert(action_ != NULL);
    action_->run_action();
}

extern "C" {
    void lightweight_makecontext(lw_ucontext_t *ucp, void (*func) (void), void *stack, size_t stack_size) {
        uint64_t *sp; /* A pointer into the stack. */

        /* Start at the beginning. */
        sp = (uint64_t *) ((uintptr_t) stack + stack_size);

        /* Align stack. The x86-64 ABI requires the stack pointer to
           always be 16-byte-aligned at function calls.  That is,
           "(%rsp - 8) is always a multiple of 16 when control is
           transferred to the function entry point". */
        sp = (uint64_t *) (((uintptr_t) sp) & -16L);

        // Currently sp is 16-byte aligned.

        /* Set up the instruction pointer; this will be popped off the stack by
         * ret in swapcontext once all the other registers have been "restored". */
        sp--;
        sp--;

        // Subtracted 2*sizeof(int64_t), so sp is still 16-byte aligned.

        *sp = (uint64_t) func;

        /* These registers (r12, r13, r14, r15, rbx, rbp) are going to be
         * popped off the stack by swapcontext; they're callee-saved, so
         * whatever happens to be in them will be ignored. */
        sp -= 6;

        // Subtracted 6*sizeof(int64_t), so sp is still 16-byte aligned.

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

/* The coroutine we're currently in, if any. NULL if we are in the main context. */
static __thread coro_t *current_coro = NULL;

/* The main context. */
static __thread lw_ucontext_t scheduler;

/* The previous context. */
static __thread coro_t *prev_coro = NULL;

/* A list of coro_t objects that are not in use. */
static __thread intrusive_list_t<coro_t> *free_coros = NULL;

#ifndef NDEBUG

/* An integer counting the number of coros on this thread */
static __thread int coro_count = 0;

/* These variables are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING`. They record the number of things that are currently
preventing us from `wait()`ing or `notify_now()`ing or whatever. */
__thread int assert_no_coro_waiting_counter = 0;
__thread int assert_finite_coro_waiting_counter = 0;

#endif  // NDEBUG

/* coro_globals_t */

coro_globals_t::coro_globals_t() {
    rassert(!current_coro);

    rassert(free_coros == NULL);
    free_coros = new intrusive_list_t<coro_t>;
}

coro_globals_t::~coro_globals_t() {
    rassert(!current_coro);

    /* Destroy remaining coroutines */
    while (coro_t *s = free_coros->head()) {
        free_coros->remove(s);
        delete s;
    }

    delete free_coros;
    free_coros = NULL;
}

/* coro_t */

coro_t::coro_t() : 
    home_thread_(get_thread_id()),
    notified_(false),
    waiting_(false)
{
    pm_allocated_coroutines++;

#ifndef NDEBUG
    coro_count++;
#endif

    rassert(coro_count < MAX_COROS_PER_THREAD, "Too many coroutines "
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
    lightweight_makecontext(&env, &coro_t::run, stack, coro_stack_size);
}

void coro_t::run() {
    coro_t *self = current_coro;

    /* Make sure we're on the right stack. */
#ifndef NDEBUG
    char dummy;
    rassert(&dummy >= (char*)self->stack);
    rassert(&dummy < (char*)self->stack + coro_stack_size);
#endif

    while (true) {
        rassert(current_coro == self);
        rassert(self->notified_ == false);
        rassert(self->waiting_ == true);
        self->waiting_ = false;

        self->action_wrapper.run();

        self->action_wrapper.reset();
        /* Return the context to the free-contexts list we took it from. */
        do_on_thread(self->home_thread_, boost::bind(return_coro_to_free_coros, self));
        pm_active_coroutines++;

        if (prev_coro) {
            lightweight_swapcontext(&self->env, prev_coro->env);
        } else {
            lightweight_swapcontext(&self->env, scheduler);
        }
    }
}

coro_t::~coro_t() {
#ifdef VALGRIND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#pragma GCC diagnostic pop
#endif  // VALGRIND

    mprotect(stack, getpagesize(), PROT_READ|PROT_WRITE); //Undo protections changes

    free(stack);

    pm_allocated_coroutines--;

#ifndef NDEBUG
    coro_count--;
#endif
}

/* coro_t */

coro_t * coro_t::get_coro(int thread) {
    coro_t *coro;

    assert_good_thread_id(thread);
    pm_active_coroutines++;

    /* Find us a stack */
    if (free_coros->size() == 0) {
        coro = new coro_t();
    } else {
        coro = free_coros->tail();
        free_coros->remove(coro);
    }

    coro->current_thread_ = thread;
    coro->notified_ = false;
    coro->waiting_ = true;

    return coro;
}

void coro_t::return_coro_to_free_coros(coro_t *coro) {
    free_coros->push_back(coro);
}

coro_t *coro_t::self() {   /* class method */
    return current_coro;
}

void coro_t::wait() {   /* class method */
    rassert(self(), "Not in a coroutine context");
    rassert(assert_no_coro_waiting_counter == 0 &&
            assert_finite_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().");

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
        lightweight_swapcontext(&current_coro->env, prev_coro->env);
    } else {
        lightweight_swapcontext(&current_coro->env, scheduler);
    }

    rassert(current_coro);
    rassert(self()->current_thread_ == get_thread_id());

    rassert(current_coro->waiting_);
    current_coro->waiting_ = false;
}

void coro_t::yield() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify_later();
    self()->wait();
}

void coro_t::notify_now() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_ == get_thread_id());

#ifndef NDEBUG
    rassert(assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use notify_now() or spawn_now().");

    /* Record old value of `assert_finite_coro_waiting_counter`. It must be legal to call
    `coro_t::wait()` within the coro we are going to jump to, or else we would never jump
    back. */
    int old_assert_finite_coro_waiting_counter = assert_finite_coro_waiting_counter;
    assert_finite_coro_waiting_counter = 0;
#endif

#ifndef NDEBUG
    /* It's not safe to notify_now() in the catch-clause of an exception handler for the same
    reason we can't wait(). */
    rassert(!abi::__cxa_current_exception_type());
#endif

    coro_t *prev_prev_coro = prev_coro;
    prev_coro = current_coro;
    current_coro = this;

    if (prev_coro) {
        lightweight_swapcontext(&prev_coro->env, env);
    } else {
        lightweight_swapcontext(&scheduler, env);
    }

    rassert(this->current_thread_ == get_thread_id());
    rassert(current_coro == this);
    current_coro = prev_coro;
    prev_coro = prev_prev_coro;

#ifndef NDEBUG
    /* Restore old value of `assert_finite_coro_waiting_counter`. */
    assert_finite_coro_waiting_counter = old_assert_finite_coro_waiting_counter;
#endif
}

void coro_t::notify_later() {
    rassert(!notified_);
    notified_ = true;

    /* notify_later() doesn't switch to the coroutine immediately; instead, it just pushes
    the coroutine onto the event queue. */

    /* current_thread is the thread that the coroutine lives on, which may or may not be the
    same as get_thread_id(). */
    linux_thread_pool_t::thread->message_hub.store_message(current_thread_, this);
}

void coro_t::move_to_thread(int thread) {   /* class method */
    assert_good_thread_id(thread);
    if (thread == get_thread_id()) {
        // If we're trying to switch to the thread we're currently on, do nothing.
        return;
    }
    rassert(coro_t::self(), "coro_t::move_to_thread() called when not in a coroutine, and the "
        "desired thread isn't the one we're already on.");
    self()->current_thread_ = thread;
    self()->notify_later();
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
    return current_coro && current_coro->stack == base;
}

#ifndef NDEBUG

/* These are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING` */
assert_no_coro_waiting_t::assert_no_coro_waiting_t() {
    assert_no_coro_waiting_counter++;
}
assert_no_coro_waiting_t::~assert_no_coro_waiting_t() {
    assert_no_coro_waiting_counter--;
}
assert_finite_coro_waiting_t::assert_finite_coro_waiting_t() {
    assert_finite_coro_waiting_counter++;
}
assert_finite_coro_waiting_t::~assert_finite_coro_waiting_t() {
    assert_finite_coro_waiting_counter--;
}

#endif /* NDEBUG */
