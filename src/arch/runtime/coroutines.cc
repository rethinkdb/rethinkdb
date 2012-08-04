#include "arch/runtime/coroutines.hpp"

#include <stdio.h>
#include <string.h>

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#include <stack>
#endif

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "do_on_thread.hpp"

#include "perfmon/perfmon.hpp"
#include "utils.hpp"

static perfmon_counter_t pm_active_coroutines, pm_allocated_coroutines;
static perfmon_multi_membership_t pm_coroutines_membership(&get_global_perfmon_collection(),
    &pm_active_coroutines, "active_coroutines",
    &pm_allocated_coroutines, "allocated_coroutines",
    NULL);

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* `coro_globals_t` holds all of the thread-local variables that coroutines need
to operate. There is one per thread; it is constructed by the constructor for
`coro_runtime_t` and destroyed by the destructor. If one exists, you can find
it in `cglobals`. */

struct coro_globals_t {

    /* The coroutine we're currently in, if any. NULL if we are in the main context. */
    coro_t *current_coro;

    /* The main context. */
    context_ref_t scheduler;

    /* The previous context. */
    coro_t *prev_coro;

    /* A list of coro_t objects that are not in use. */
    intrusive_list_t<coro_t> free_coros;

#ifndef NDEBUG

    /* An integer counting the number of coros on this thread */
    int coro_count;

    /* These variables are used in the implementation of
    `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING`. They record the
    number of things that are currently preventing us from `wait()`ing or
    `notify_now_deprecated()`ing or whatever. */
    int assert_no_coro_waiting_counter;
    std::stack<std::pair<std::string, int> > no_waiting_call_sites;

    int assert_finite_coro_waiting_counter;
    std::stack<std::pair<std::string, int> > finite_waiting_call_sites;

    std::map<std::string, size_t> running_coroutine_counts;
    std::map<std::string, size_t> total_coroutine_counts;

#endif  // NDEBUG

    coro_globals_t()
        : current_coro(NULL)
        , prev_coro(NULL)
#ifndef NDEBUG
        , coro_count(0)
        , assert_no_coro_waiting_counter(0)
        , assert_finite_coro_waiting_counter(0)
#endif
        { }

    ~coro_globals_t() {
        /* We shouldn't be shutting down from within a coroutine */
        rassert(!current_coro);

        /* Destroy remaining coroutines */
        while (coro_t *s = free_coros.head()) {
            free_coros.remove(s);
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

#ifndef NDEBUG
void coro_runtime_t::get_coroutine_counts(std::map<std::string, size_t> *dest) {
    dest->clear();
    dest->insert(cglobals->total_coroutine_counts.begin(), cglobals->total_coroutine_counts.end());
}
#endif

/* coro_t */

#ifndef NDEBUG
static __thread int64_t coro_selfname_counter = 0;
#endif

coro_t::coro_t() :
    stack(&coro_t::run, coro_stack_size),
    current_thread_(linux_thread_pool_t::thread_id),
    notified_(false),
    waiting_(false)
#ifndef NDEBUG
    , selfname_number(get_thread_id() + MAX_THREADS * ++coro_selfname_counter)
#endif
{
    ++pm_allocated_coroutines;

#ifndef NDEBUG
    ++cglobals->coro_count;
    rassert(cglobals->coro_count < MAX_COROS_PER_THREAD, "Too many "
            "coroutines allocated on this thread. This is problem due to a "
            "misuse of the coroutines\n");
#endif
}

void coro_t::return_coro_to_free_list(coro_t *coro) {
    cglobals->free_coros.push_back(coro);
}

coro_t::~coro_t() {
    /* We never move contexts from one thread to another any more. */
    rassert(get_thread_id() == home_thread());

#ifndef NDEBUG
    --cglobals->coro_count;
#endif
    --pm_allocated_coroutines;
}

void coro_t::run() {
    coro_t *coro = cglobals->current_coro;

#ifndef NDEBUG
    char dummy;  /* Make sure we're on the right stack. */
    rassert(coro->stack.address_in_stack(&dummy));
#endif

    while (true) {
        rassert(coro == cglobals->current_coro);
        rassert(coro->current_thread_ == get_thread_id());
        rassert(coro->notified_ == false);
        rassert(coro->waiting_ == true);
        coro->waiting_ = false;

#ifndef NDEBUG
        // Keep track of how many coroutines of each type ran
        cglobals->running_coroutine_counts[coro->coroutine_type.c_str()]++;
        cglobals->total_coroutine_counts[coro->coroutine_type.c_str()]++;
#endif
        coro->action_wrapper.run();
#ifndef NDEBUG
        // Pet the watchdog to reset it before execution moves
        pet_watchdog();
        cglobals->running_coroutine_counts[coro->coroutine_type.c_str()]--;
#endif

        rassert(coro->current_thread_ == get_thread_id());

        // Destroy the Callable object which was either allocated within the coro_t or on the heap
        coro->action_wrapper.reset();

        /* Return the context to the free-contexts list we took it from. */
        do_on_thread(coro->home_thread(), boost::bind(coro_t::return_coro_to_free_list, coro));
        --pm_active_coroutines;

        if (cglobals->prev_coro) {
            context_switch(&coro->stack.context, &cglobals->prev_coro->stack.context);
        } else {
            context_switch(&coro->stack.context, &cglobals->scheduler);
        }
    }
}

#ifndef NDEBUG
// This function parses out the type that a coroutine was created with (usually a boost::bind), by
//  parsing it out of the __PRETTY_FUNCTION__ string of a templated function.  This makes some
//  assumptions about the format of that string, but if get_and_init_coro is changed, this may
//  need to be updated.  The only reason we do this is so we don't have to enable RTTI to figure
//  out the type of the template, but we can still provide a clean typename.
void coro_t::parse_coroutine_type(const char *coroutine_function)
{
     coroutine_type.assign(coroutine_function);
}
#endif

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

#ifndef NDEBUG
        // Pet the watchdog to reset it before execution moves
        pet_watchdog();
#endif

    if (cglobals->prev_coro) {
        context_switch(&self()->stack.context, &cglobals->prev_coro->stack.context);
    } else {
        context_switch(&self()->stack.context, &cglobals->scheduler);
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

void coro_t::notify_now_deprecated() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_ == linux_thread_pool_t::thread_id);

#ifndef NDEBUG
    rassert(cglobals->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use notify_now_deprecated() or spawn_now_dangerously().");

    /* Record old value of `assert_finite_coro_waiting_counter`. It must be legal to call
    `coro_t::wait()` within the coro we are going to jump to, or else we would never jump
    back. */
    int old_assert_finite_coro_waiting_counter = cglobals->assert_finite_coro_waiting_counter;
    cglobals->assert_finite_coro_waiting_counter = 0;
#endif

#ifndef NDEBUG
    /* It's not safe to notify_now_deprecated() in the catch-clause of an exception handler for the same
    reason we can't wait(). */
    rassert(!abi::__cxa_current_exception_type());
#endif

    coro_t *prev_prev_coro = cglobals->prev_coro;
    cglobals->prev_coro = cglobals->current_coro;
    cglobals->current_coro = this;

#ifndef NDEBUG
    // Pet the watchdog to reset it before execution moves
    pet_watchdog();
#endif

    if (cglobals->prev_coro) {
        context_switch(&cglobals->prev_coro->stack.context, &this->stack.context);
    } else {
        context_switch(&cglobals->scheduler, &this->stack.context);
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
    // TODO: implement a notify_any_thread function if this is changed to not be thread-safe
    // rassert(current_thread_ == linux_thread_pool_t::thread_id);

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
    rassert(coro_t::self(), "coro_t::move_to_thread() called when not in a coroutine.");
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

    /* TODO: When `notify_now_deprecated()` is finally removed, just fold it
    into this function. */
    notify_now_deprecated();
}

void coro_t::set_coroutine_stack_size(size_t size) {
    coro_stack_size = size;
}

artificial_stack_t* coro_t::get_stack() {
    return &stack;
}

/* Called by SIGSEGV handler to identify segfaults that come from overflowing a coroutine's
stack. Could also in theory be used by a function to check if it's about to overflow
the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    return cglobals->current_coro && cglobals->current_coro->stack.address_is_stack_overflow(addr);
}

bool coroutines_have_been_initialized() {
    return cglobals != NULL;
}

coro_t * coro_t::get_coro() {
    rassert(coroutines_have_been_initialized());
    coro_t *coro;

    if (cglobals->free_coros.size() == 0) {
        coro = new coro_t();
    } else {
        coro = cglobals->free_coros.tail();
        cglobals->free_coros.remove(coro);
    }

    rassert(!coro->intrusive_list_node_t<coro_t>::in_a_list());

    coro->current_thread_ = get_thread_id();
    coro->notified_ = false;
    coro->waiting_ = true;

    ++pm_active_coroutines;
    return coro;
}

#ifndef NDEBUG

/* These are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING` */
assert_no_coro_waiting_t::assert_no_coro_waiting_t(const std::string& filename, int line_no) {
    cglobals->no_waiting_call_sites.push(std::make_pair(filename, line_no));
    ++cglobals->assert_no_coro_waiting_counter;
}
assert_no_coro_waiting_t::~assert_no_coro_waiting_t() {
    cglobals->no_waiting_call_sites.pop();
    --cglobals->assert_no_coro_waiting_counter;
}
assert_finite_coro_waiting_t::assert_finite_coro_waiting_t(const std::string& filename, int line_no) {
    cglobals->finite_waiting_call_sites.push(std::make_pair(filename, line_no));
    ++cglobals->assert_finite_coro_waiting_counter;
}
assert_finite_coro_waiting_t::~assert_finite_coro_waiting_t() {
    cglobals->finite_waiting_call_sites.pop();
    --cglobals->assert_finite_coro_waiting_counter;
}

home_coro_mixin_t::home_coro_mixin_t()
    : home_coro(coro_t::self())
{ }

void home_coro_mixin_t::assert_coro() {
    rassert(home_coro == coro_t::self());
}

#endif /* NDEBUG */
