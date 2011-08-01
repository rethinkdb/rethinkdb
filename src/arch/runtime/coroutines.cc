#include "arch/runtime/coroutines.hpp"


#include <stdio.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "config/args.hpp"
#include "do_on_thread.hpp"

#include "perfmon.hpp"
#include "utils.hpp"

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#endif

perfmon_counter_t pm_active_coroutines("active_coroutines"),
                  pm_allocated_coroutines("allocated_coroutines");

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* coro_context_t is only used internally within the coroutine logic. For performance reasons,
we recycle stacks and ucontexts; the coro_context_t represents a stack and a ucontext. */

struct coro_context_t : public intrusive_list_node_t<coro_context_t> {
    coro_context_t();

    /* The run() function is at the bottom of every coro_context_t's call stack. It repeatedly
    waits for a coroutine to run and then calls that coroutine's run() method. */
    static void run();

    ~coro_context_t();

    /* The guts of the coro_context_t */
    artificial_stack_t stack;
};

/* The coroutine we're currently in, if any. NULL if we are in the main context. */
static __thread coro_t *current_coro = NULL;

/* The main context. */
static __thread context_ref_t *scheduler = NULL;

/* The previous context. */
static __thread coro_t *prev_coro = NULL;

/* A list of coro_context_t objects that are not in use. */
static __thread intrusive_list_t<coro_context_t> *free_contexts = NULL;

#ifndef NDEBUG

/* An integer counting the number of coros on this thread */
static __thread int coro_context_count = 0;

/* These variables are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING`. They record the number of things that are currently
preventing us from `wait()`ing or `notify_now()`ing or whatever. */
__thread int assert_no_coro_waiting_counter = 0;
__thread int assert_finite_coro_waiting_counter = 0;

#endif  // NDEBUG

/* coro_globals_t */

coro_globals_t::coro_globals_t() {
    rassert(!current_coro);

    /* `context_ref_t` is not a POD type, so we have to allocate it and then
    deallocate it later. */
    rassert(!scheduler);
    scheduler = new context_ref_t;

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

    delete scheduler;
}

/* coro_context_t */

coro_context_t::coro_context_t() :
    stack(&coro_context_t::run, coro_stack_size)
{
    pm_allocated_coroutines++;

#ifndef NDEBUG
    coro_context_count++;
    rassert(coro_context_count < MAX_COROS_PER_THREAD, "Too many coroutines "
            "allocated on this thread. This is problem due to a misuse of the "
            "coroutines\n");
#endif
}

void coro_context_t::run() {
    coro_context_t *self = current_coro->context;

    /* Make sure we're on the right stack. */
#ifndef NDEBUG
    char dummy;
    rassert(self->stack.address_in_stack(&dummy));
#endif

    while (true) {
        current_coro->run();

        if (prev_coro) {
            context_switch(&self->stack.context, &prev_coro->context->stack.context);
        } else {
            context_switch(&self->stack.context, scheduler);
        }
    }
}

coro_context_t::~coro_context_t() {
#ifndef NDEBUG
    coro_context_count--;
#endif
    pm_allocated_coroutines--;
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
        context_switch(&current_coro->context->stack.context, &prev_coro->context->stack.context);
    } else {
        context_switch(&current_coro->context->stack.context, scheduler);
    }

    rassert(current_coro);

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
    rassert(current_thread_ == linux_thread_pool_t::thread_id);

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
        context_switch(&prev_coro->context->stack.context, &this->context->stack.context);
    } else {
        context_switch(scheduler, &this->context->stack.context);
    }

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
    if (thread == linux_thread_pool_t::thread_id) {
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

artificial_stack_t* coro_t::get_stack() {
    return &context->stack;
}

/* Called by SIGSEGV handler to identify segfaults that come from overflowing a coroutine's
stack. Could also in theory be used by a function to check if it's about to overflow
the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    return current_coro && current_coro->context->stack.address_is_stack_overflow(addr);
}

void coro_t::spawn_later(const boost::function<void()>& deed) {
    spawn_on_thread(linux_thread_pool_t::thread_id, deed);
}

void coro_t::spawn_now(const boost::function<void()> &deed) {
    (new coro_t(deed, linux_thread_pool_t::thread_id))->notify_now();
}

void coro_t::spawn_on_thread(int thread, const boost::function<void()>& deed) {
    (new coro_t(deed, thread))->notify_later();
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
