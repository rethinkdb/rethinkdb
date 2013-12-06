// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/coroutines.hpp"

#include <stdio.h>
#include <string.h>

#ifndef NDEBUG
#include <stack>   /* the data structure, not the run-time concept */
#endif

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "arch/runtime/runtime.hpp"
#include "config/args.hpp"
#include "do_on_thread.hpp"
#include "thread_local.hpp"

#include "perfmon/perfmon.hpp"
#include "utils.hpp"
#include "rethinkdb_backtrace.hpp"
#include "arch/runtime/coro_profiler.hpp"

static perfmon_counter_t pm_active_coroutines, pm_allocated_coroutines;
static perfmon_multi_membership_t pm_coroutines_membership(&get_global_perfmon_collection(),
    &pm_active_coroutines, "active_coroutines",
    &pm_allocated_coroutines, "allocated_coroutines",
    NULLPTR);

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* `coro_globals_t` holds all of the thread-local variables that coroutines need
to operate. There is one per thread; it is constructed by the constructor for
`coro_runtime_t` and destroyed by the destructor. If one exists, you can find
it in `cglobals`. */

struct coro_globals_t {

    /* The coroutine we're currently in, if any. NULL if we are in the main context. */
    coro_t *current_coro;

    /* The main context. */
    coro_context_ref_t scheduler;

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

    std::set<coro_t*> active_coroutines;

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

TLS(coro_globals_t *, cglobals);

coro_runtime_t::coro_runtime_t() {
    rassert(!TLS_get_cglobals(), "coro runtime initialized twice on this thread");
    TLS_set_cglobals(new coro_globals_t);
}

coro_runtime_t::~coro_runtime_t() {
    rassert(TLS_get_cglobals());
    delete TLS_get_cglobals();
    TLS_set_cglobals(NULL);
}

#ifndef NDEBUG
void coro_runtime_t::get_coroutine_counts(std::map<std::string, size_t> *dest) {
    dest->clear();
    dest->insert(TLS_get_cglobals()->total_coroutine_counts.begin(), TLS_get_cglobals()->total_coroutine_counts.end());
}
#endif

/* coro_t */

#ifndef NDEBUG
TLS_with_init(int64_t, coro_selfname_counter, 0);
#endif

coro_t::coro_t() :
    stack(&coro_t::run, coro_stack_size),
    current_thread_(linux_thread_pool_t::get_thread_id()),
    notified_(false),
    waiting_(false)
#ifndef NDEBUG
    , selfname_number(get_thread_id().threadnum + MAX_THREADS *
          // The comma here is the comma operator, to implement the semantics
          // of `++coro_selfname_counter`
          (TLS_set_coro_selfname_counter(TLS_get_coro_selfname_counter()+1),
           TLS_get_coro_selfname_counter()))
#endif
#ifdef CROSS_CORO_BACKTRACES
    , spawn_backtrace_size(0)
#endif
{
    ++pm_allocated_coroutines;

#ifndef NDEBUG
    TLS_get_cglobals()->coro_count++;
    rassert(TLS_get_cglobals()->coro_count < MAX_COROS_PER_THREAD, "Too many "
            "coroutines allocated on this thread. This is problem due to a "
            "misuse of the coroutines\n");
#endif
}

void coro_t::return_coro_to_free_list(coro_t *coro) {
    TLS_get_cglobals()->free_coros.push_back(coro);
}

coro_t::~coro_t() {
    /* We never move contexts from one thread to another any more. */
    rassert(get_thread_id() == home_thread());

#ifndef NDEBUG
    TLS_get_cglobals()->coro_count--;
#endif
    --pm_allocated_coroutines;
}

void coro_t::run() {
    coro_t *coro = TLS_get_cglobals()->current_coro;

#ifndef NDEBUG
    char dummy;  /* Make sure we're on the right stack. */
    rassert(coro->stack.address_in_stack(&dummy));
#endif

    while (true) {
        rassert(coro == TLS_get_cglobals()->current_coro);
        rassert(coro->current_thread_ == get_thread_id());
        rassert(coro->notified_ == false);
        rassert(coro->waiting_ == true);
        coro->waiting_ = false;

#ifndef NDEBUG
        // Keep track of how many coroutines of each type ran
        TLS_get_cglobals()->running_coroutine_counts[coro->coroutine_type.c_str()]++;
        TLS_get_cglobals()->total_coroutine_counts[coro->coroutine_type.c_str()]++;
        TLS_get_cglobals()->active_coroutines.insert(coro);
#endif
        PROFILER_CORO_RESUME;
        coro->action_wrapper.run();
        PROFILER_CORO_YIELD(0);
#ifndef NDEBUG
        TLS_get_cglobals()->running_coroutine_counts[coro->coroutine_type.c_str()]--;
        TLS_get_cglobals()->active_coroutines.erase(coro);
#endif

        rassert(coro->current_thread_ == get_thread_id());

        // Destroy the Callable object which was either allocated within the coro_t or on the heap
        coro->action_wrapper.reset();

        /* Return the context to the free-contexts list we took it from. */
        do_on_thread(coro->home_thread(), boost::bind(coro_t::return_coro_to_free_list, coro));
        --pm_active_coroutines;

        coro_globals_t *my_cglobals = TLS_get_cglobals();
        if (my_cglobals->prev_coro) {
            context_switch(&coro->stack.context, &my_cglobals->prev_coro->stack.context);
        } else {
            context_switch(&coro->stack.context, &my_cglobals->scheduler);
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
    coro_globals_t *my_cglobals = TLS_get_cglobals();
    return my_cglobals == NULL ? NULL : my_cglobals->current_coro;
}

void coro_t::wait() {   /* class method */
    coro_t *coro = self();

    rassert(coro, "Not in a coroutine context");
    rassert(TLS_get_cglobals()->assert_finite_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d",
        TLS_get_cglobals()->finite_waiting_call_sites.top().first.c_str(), TLS_get_cglobals()->finite_waiting_call_sites.top().second);

    rassert(TLS_get_cglobals()->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d",
        TLS_get_cglobals()->no_waiting_call_sites.top().first.c_str(), TLS_get_cglobals()->no_waiting_call_sites.top().second);

    rassert(!coro->waiting_);
    coro->waiting_ = true;

    PROFILER_CORO_YIELD(1);
    coro_globals_t *my_cglobals = TLS_get_cglobals();
    if (my_cglobals->prev_coro) {
        context_switch(&self()->stack.context, &my_cglobals->prev_coro->stack.context);
    } else {
        context_switch(&self()->stack.context, &my_cglobals->scheduler);
    }
    PROFILER_CORO_RESUME;

    rassert(self() == coro);
    rassert(coro->waiting_);
    coro->waiting_ = false;
}

void coro_t::yield() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify_later_ordered();
    self()->wait();
}

void coro_t::notify_now_deprecated() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_.threadnum == linux_thread_pool_t::get_thread_id());

    coro_globals_t *my_cglobals = TLS_get_cglobals();

#ifndef NDEBUG
    rassert(my_cglobals->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use notify_now_deprecated() or spawn_now_dangerously().");

    /* Record old value of `assert_finite_coro_waiting_counter`. It must be legal to call
    `coro_t::wait()` within the coro we are going to jump to, or else we would never jump
    back. */
    int old_assert_finite_coro_waiting_counter = my_cglobals->assert_finite_coro_waiting_counter;
    my_cglobals->assert_finite_coro_waiting_counter = 0;
#endif

    if (self() != NULL) {
        PROFILER_CORO_YIELD(1);
    }
    coro_t *prev_prev_coro = my_cglobals->prev_coro;
    my_cglobals->prev_coro = my_cglobals->current_coro;
    my_cglobals->current_coro = this;

    if (my_cglobals->prev_coro) {
        context_switch(&my_cglobals->prev_coro->stack.context, &this->stack.context);
    } else {
        context_switch(&my_cglobals->scheduler, &this->stack.context);
    }

    // The thread might have changed, and with it the cglobals we have to access
    my_cglobals = TLS_get_cglobals();
    rassert(my_cglobals->current_coro == this);
    my_cglobals->current_coro = my_cglobals->prev_coro;
    my_cglobals->prev_coro = prev_prev_coro;
    if (coro_t::self() != NULL) {
        PROFILER_CORO_RESUME;
    }

#ifndef NDEBUG
    /* Restore old value of `assert_finite_coro_waiting_counter`. */
    my_cglobals->assert_finite_coro_waiting_counter = old_assert_finite_coro_waiting_counter;
#endif
}

void coro_t::notify_sometime() {
    rassert(!notified_);
    notified_ = true;
    linux_thread_pool_t::get_thread()->message_hub.store_message_sometime(
        current_thread_,
        this);
}

void coro_t::notify_later_ordered() {
    rassert(!notified_);
    notified_ = true;

    /* `current_thread` is the thread that the coroutine lives on, which may or may not be the
    same as `get_thread_id()`.  (In a call to move_to_thread, it won't be.) */
    linux_thread_pool_t::get_thread()->message_hub.store_message_ordered(
        current_thread_,
        this);
}

void coro_t::move_to_thread(threadnum_t thread) {
    assert_good_thread_id(thread);
    rassert(coro_t::self(), "coro_t::move_to_thread() called when not in a coroutine.");
    if (thread.threadnum == linux_thread_pool_t::get_thread_id()) {
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

coro_stack_t* coro_t::get_stack() {
    return &stack;
}

/* Called by SIGSEGV handler to identify segfaults that come from overflowing a coroutine's
stack. Could also in theory be used by a function to check if it's about to overflow
the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    return TLS_get_cglobals()->current_coro && TLS_get_cglobals()->current_coro->stack.address_is_stack_overflow(addr);
}

bool coroutines_have_been_initialized() {
    return TLS_get_cglobals() != NULL;
}

coro_t * coro_t::get_coro() {
    rassert(coroutines_have_been_initialized());
    coro_t *coro;

    coro_globals_t *my_cglobals = TLS_get_cglobals();

    if (my_cglobals->free_coros.size() == 0) {
        coro = new coro_t();
    } else {
        coro = my_cglobals->free_coros.tail();
        my_cglobals->free_coros.remove(coro);
    }

    rassert(!coro->intrusive_list_node_t<coro_t>::in_a_list());

    coro->current_thread_ = get_thread_id();
    coro->notified_ = false;
    coro->waiting_ = true;

    ++pm_active_coroutines;
    return coro;
}

void coro_t::grab_spawn_backtrace() {
#ifdef CROSS_CORO_BACKTRACES
    // Skip a few constant frames at the beginning of the backtrace.
    // This is purely a cosmetic thing, to make the backtraces
    // nicer to look at and easier to read.
    //
    // The +2 comes together as follows:
    // +1 for our own stack frame (`grab_spawn_backtrace()`)
    // +1 for the fact that we get called from `get_and_init_coro()`
    //    which is also not interesting at all.
    const int num_frames_to_skip = NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE + 2;

    const size_t buffer_size = static_cast<size_t>(CROSS_CORO_BACKTRACES_MAX_SIZE
                                                   + num_frames_to_skip);
    void *buffer[buffer_size];
    spawn_backtrace_size = rethinkdb_backtrace(buffer, buffer_size);
    if (spawn_backtrace_size < num_frames_to_skip) {
        // Something is fishy if this happens... Probably RethinkDB was compiled
        // with some optimizations enabled that inlined some functions or
        // optimized away some stack frames, or the value of
        // NUM_FRAMES_INSIDE_RETHINKDB_BACKTRACE is wrong, or we were called
        // from an unexpected context.
        rassert(spawn_backtrace_size >= num_frames_to_skip,
            "Got an unexpected backtrace. See comment in coro_t::grab_spawn_backtrace(). "
            "If you determine that this is acceptable in your specific case (e.g. "
            "you are just debugging something), feel free to temporarily comment "
            "this assertion.");
        spawn_backtrace_size = num_frames_to_skip;
    }
    spawn_backtrace_size -= num_frames_to_skip;
    rassert(spawn_backtrace_size >= 0);
    rassert(spawn_backtrace_size <= CROSS_CORO_BACKTRACES_MAX_SIZE);
    memcpy(spawn_backtrace, buffer + num_frames_to_skip, spawn_backtrace_size * sizeof(void *));

    // If we have space left and were called from inside a coroutine,
    // we also append the parent's spawn_backtrace.
    int space_remaining = CROSS_CORO_BACKTRACES_MAX_SIZE - spawn_backtrace_size;
    if (space_remaining > 0 && self() != NULL) {
        spawn_backtrace_size += self()->copy_spawn_backtrace(spawn_backtrace + spawn_backtrace_size,
                                                             space_remaining);
    }
#endif
}

#ifdef CROSS_CORO_BACKTRACES
int coro_t::copy_spawn_backtrace(void **buffer_out, int size) const {
#else
int coro_t::copy_spawn_backtrace(void **, int) const {
#endif
#ifdef CROSS_CORO_BACKTRACES
    guarantee(buffer_out != NULL);
    guarantee(size >= 0);
    int num_frames_to_store = std::min(size, spawn_backtrace_size);
    memcpy(buffer_out,
           spawn_backtrace,
           static_cast<size_t>(num_frames_to_store) * sizeof(void *));
    return num_frames_to_store;
#else
    return 0;
#endif
}

#ifndef NDEBUG

/* These are used in the implementation of `ASSERT_NO_CORO_WAITING` and
`ASSERT_FINITE_CORO_WAITING` */
assert_no_coro_waiting_t::assert_no_coro_waiting_t(const std::string& filename, int line_no) {
    TLS_get_cglobals()->no_waiting_call_sites.push(std::make_pair(filename, line_no));
    TLS_get_cglobals()->assert_no_coro_waiting_counter++;
}
assert_no_coro_waiting_t::~assert_no_coro_waiting_t() {
    TLS_get_cglobals()->no_waiting_call_sites.pop();
    TLS_get_cglobals()->assert_no_coro_waiting_counter--;
}
assert_finite_coro_waiting_t::assert_finite_coro_waiting_t(const std::string& filename, int line_no) {
    TLS_get_cglobals()->finite_waiting_call_sites.push(std::make_pair(filename, line_no));
    TLS_get_cglobals()->assert_finite_coro_waiting_counter++;
}
assert_finite_coro_waiting_t::~assert_finite_coro_waiting_t() {
    TLS_get_cglobals()->finite_waiting_call_sites.pop();
    TLS_get_cglobals()->assert_finite_coro_waiting_counter--;
}

home_coro_mixin_t::home_coro_mixin_t()
    : home_coro(coro_t::self())
{ }

void home_coro_mixin_t::assert_coro() {
    rassert(home_coro == coro_t::self());
}

#endif /* NDEBUG */
