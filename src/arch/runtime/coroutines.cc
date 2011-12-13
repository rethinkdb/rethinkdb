#include "arch/runtime/runtime.hpp"


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
#include <stack>
#endif

perfmon_counter_t pm_active_coroutines("active_coroutines"),
                  pm_allocated_coroutines("allocated_coroutines");

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* coro_context_t is only used internally within the coroutine logic. For performance reasons,
we recycle stacks and ucontexts; the coro_context_t represents a stack and a ucontext. */

struct coro_context_t :
    public intrusive_list_node_t<coro_context_t>,
    public home_thread_mixin_t
{
    coro_context_t();

    /* The run() function is at the bottom of every coro_context_t's call stack. It repeatedly
    waits for a coroutine to run and then calls that coroutine's run() method. */
    static void run();

    ~coro_context_t();

    /* The guts of the coro_context_t */
    artificial_stack_t stack;
};

/* `coro_globals_t` holds all of the thread-local variables that coroutines need
to operate. There is one per thread; it is constructed by the constructor for
`coro_runtime_t` and destroyed by the destructor. If one exists, you can find
it in `cglobals`. */

struct coro_globals_t {

    /* The coroutine we're currently in, if any. NULL if we are in the main
    context. */
    coro_t *current_coro;

    /* The main context. */
    context_ref_t scheduler;

    /* The previous context. */
    coro_t *prev_coro;

    /* A list of coro_context_t objects that are not in use. */
    intrusive_list_t<coro_context_t> free_contexts;

#ifndef NDEBUG

    /* An integer counting the number of coros on this thread */
    int coro_context_count;

    /* These variables are used in the implementation of
    `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING`. They record the
    number of things that are currently preventing us from `wait()`ing or
    `notify_now()`ing or whatever. */
    int assert_no_coro_waiting_counter;
    std::stack<std::pair<std::string, int> > no_waiting_call_sites;

    int assert_finite_coro_waiting_counter;
    std::stack<std::pair<std::string, int> > finite_waiting_call_sites;

#endif  // NDEBUG

    coro_globals_t()
        : current_coro(NULL)
        , prev_coro(NULL)
#ifndef NDEBUG
        , coro_context_count(0)
        , assert_no_coro_waiting_counter(0)
        , assert_finite_coro_waiting_counter(0)
#endif
        { }

    ~coro_globals_t() {
        /* We shouldn't be shutting down from within a coroutine */
        rassert(!current_coro);

        /* Destroy remaining coroutines */
        while (coro_context_t *s = free_contexts.head()) {
            free_contexts.remove(s);
            delete s;
        }
    }

};

static __thread coro_globals_t *cglobals = NULL;

coro_runtime_t::coro_runtime_t() {
    rassert(!cglobals, "coro runtime initialized twice on this thread");
    cglobals = new coro_globals_t;
}

coro_runtime_t::~coro_runtime_t() {
    rassert(cglobals);
    delete cglobals;
    cglobals = NULL;
}

/* coro_context_t */

coro_context_t::coro_context_t()
    : stack(&coro_context_t::run, coro_stack_size)
{
    pm_allocated_coroutines++;

#ifndef NDEBUG
    cglobals->coro_context_count++;
    rassert(cglobals->coro_context_count < MAX_COROS_PER_THREAD, "Too many "
            "coroutines allocated on this thread. This is problem due to a "
            "misuse of the coroutines\n");
#endif
}

void coro_context_t::run() {
    coro_context_t *self = cglobals->current_coro->context;

    /* Make sure we're on the right stack. */
#ifndef NDEBUG
    char dummy;
    rassert(self->stack.address_in_stack(&dummy));
#endif

    while (true) {
        cglobals->current_coro->run();

        if (cglobals->prev_coro) {
            context_switch(&self->stack.context, &cglobals->prev_coro->context->stack.context);
        } else {
            context_switch(&self->stack.context, &cglobals->scheduler);
        }
    }
}

coro_context_t::~coro_context_t() {
#ifndef NDEBUG
    cglobals->coro_context_count--;
#endif
    pm_allocated_coroutines--;
}

/* coro_t */

#ifndef NDEBUG
static __thread int64_t coro_selfname_counter = 0;
#endif

coro_t::coro_t(const boost::function<void()>& deed) :
#ifndef NDEBUG
    selfname_number(get_thread_id() + MAX_THREADS * ++coro_selfname_counter),
#endif
    deed_(deed),
    current_thread_(linux_thread_pool_t::thread_id),
    notified_(false),
    waiting_(true)
{
    pm_active_coroutines++;

    /* Find us a stack */
    if (cglobals->free_contexts.size() == 0) {
        context = new coro_context_t();
    } else {
        context = cglobals->free_contexts.tail();
        cglobals->free_contexts.remove(context);
    }
}

void return_context_to_free_contexts(coro_context_t *context) {
    cglobals->free_contexts.push_back(context);
}

coro_t::~coro_t() {
    /* We never move contexts from one thread to another any more. */
    rassert(get_thread_id() == context->home_thread());

    /* Return the context to the free-contexts list we took it from. */
    do_on_thread(context->home_thread(), boost::bind(return_context_to_free_contexts, context));

    pm_active_coroutines--;
}

void coro_t::run() {
    rassert(cglobals->current_coro == this);
    waiting_ = false;

    deed_();

    delete this;
}

coro_t *coro_t::self() {   /* class method */
    return cglobals->current_coro;
}

void coro_t::wait() {   /* class method */
    rassert(self(), "Not in a coroutine context");
    rassert(cglobals->assert_finite_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d", 
        cglobals->finite_waiting_call_sites.top().first.c_str(), cglobals->finite_waiting_call_sites.top().second
        );

    rassert(cglobals->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d", 
        cglobals->no_waiting_call_sites.top().first.c_str(), cglobals->no_waiting_call_sites.top().second
        );

#ifndef NDEBUG
    /* It's not safe to wait() in a catch clause of an exception handler. We use the non-standard
    GCC-only interface "cxxabi.h" to figure out if we're in the catch clause of an exception
    handler. In C++0x we will be able to use std::current_exception() instead. */
    rassert(!abi::__cxa_current_exception_type());
#endif

    rassert(!self()->waiting_);
    self()->waiting_ = true;

    if (cglobals->prev_coro) {
        context_switch(&self()->context->stack.context, &cglobals->prev_coro->context->stack.context);
    } else {
        context_switch(&self()->context->stack.context, &cglobals->scheduler);
    }

    rassert(self());

    rassert(self()->waiting_);
    self()->waiting_ = false;
}

void coro_t::yield() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify_later_ordered();
    self()->wait();
}

void coro_t::notify_now() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_ == linux_thread_pool_t::thread_id);

#ifndef NDEBUG
    rassert(cglobals->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use notify_now() or spawn_now().");

    /* Record old value of `assert_finite_coro_waiting_counter`. It must be legal to call
    `coro_t::wait()` within the coro we are going to jump to, or else we would never jump
    back. */
    int old_assert_finite_coro_waiting_counter = cglobals->assert_finite_coro_waiting_counter;
    cglobals->assert_finite_coro_waiting_counter = 0;
#endif

#ifndef NDEBUG
    /* It's not safe to notify_now() in the catch-clause of an exception handler for the same
    reason we can't wait(). */
    rassert(!abi::__cxa_current_exception_type());
#endif

    coro_t *prev_prev_coro = cglobals->prev_coro;
    cglobals->prev_coro = cglobals->current_coro;
    cglobals->current_coro = this;

    if (cglobals->prev_coro) {
        context_switch(&cglobals->prev_coro->context->stack.context, &this->context->stack.context);
    } else {
        context_switch(&cglobals->scheduler, &this->context->stack.context);
    }

    rassert(cglobals->current_coro == this);
    cglobals->current_coro = cglobals->prev_coro;
    cglobals->prev_coro = prev_prev_coro;

#ifndef NDEBUG
    /* Restore old value of `assert_finite_coro_waiting_counter`. */
    cglobals->assert_finite_coro_waiting_counter = old_assert_finite_coro_waiting_counter;
#endif
}

void coro_t::notify_sometime() {
    rassert(current_thread_ == linux_thread_pool_t::thread_id);

    /* For now we just implement `notify_sometime()` as
    `notify_later_ordered()`, but later we could eliminate the full trip around
    the event loop. */
    notify_later_ordered();
}

void coro_t::notify_later_ordered() {
    rassert(!notified_);
    notified_ = true;

    /* `notify_later_ordered()` doesn't switch to the coroutine immediately;
    instead, it just pushes the coroutine onto the event queue. */

    /* `current_thread` is the thread that the coroutine lives on, which may or
    may not be the same as `get_thread_id()`. */
    linux_thread_pool_t::thread->message_hub.store_message(current_thread_, this);
}

void coro_t::move_to_thread(int thread) {
    assert_good_thread_id(thread);
    rassert(coro_t::self(), "coro_t::move_to_thread() called when not in a coroutine.")
    if (thread == linux_thread_pool_t::thread_id) {
        // If we're trying to switch to the thread we're currently on, do nothing.
        return;
    }
    self()->current_thread_ = thread;
    self()->notify_later_ordered();
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
    return cglobals->current_coro && cglobals->current_coro->context->stack.address_is_stack_overflow(addr);
}

void coro_t::spawn_now(const boost::function<void()> &deed) {
    (new coro_t(deed))->notify_now();
}

void coro_t::spawn_sometime(const boost::function<void()> &deed) {
    (new coro_t(deed))->notify_sometime();
}

void coro_t::spawn_later_ordered(const boost::function<void()>& deed) {
    (new coro_t(deed))->notify_later_ordered();
}

void coro_t::spawn_on_thread(int thread, const boost::function<void()>& deed) {
    if (get_thread_id() == thread) {
        coro_t::spawn_later_ordered(deed);
    } else {
        do_on_thread(thread, boost::bind(&coro_t::spawn_now, deed));
    }
}

#ifndef NDEBUG

/* These are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING` */
assert_no_coro_waiting_t::assert_no_coro_waiting_t(std::string filename, int line_no) {
    cglobals->no_waiting_call_sites.push(std::make_pair(filename, line_no));
    cglobals->assert_no_coro_waiting_counter++;
}
assert_no_coro_waiting_t::~assert_no_coro_waiting_t() {
    cglobals->no_waiting_call_sites.pop();
    cglobals->assert_no_coro_waiting_counter--;
}
assert_finite_coro_waiting_t::assert_finite_coro_waiting_t(std::string filename, int line_no) {
    cglobals->finite_waiting_call_sites.push(std::make_pair(filename, line_no));
    cglobals->assert_finite_coro_waiting_counter++;
}
assert_finite_coro_waiting_t::~assert_finite_coro_waiting_t() {
    cglobals->finite_waiting_call_sites.pop();
    cglobals->assert_finite_coro_waiting_counter--;
}

#endif /* NDEBUG */
