// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "arch/runtime/coroutines.hpp"

#include <stdio.h>
#include <string.h>

#include <functional>
#ifndef NDEBUG
#include <map>
#include <set>
#include <stack>
#endif

#include "arch/runtime/context_switching.hpp"
#include "arch/runtime/coro_profiler.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "config/args.hpp"
#include "debug.hpp"
#include "do_on_thread.hpp"
#include "logger.hpp"
#include "perfmon/perfmon.hpp"
#include "rethinkdb_backtrace.hpp"
#include "thread_local.hpp"
#include "utils.hpp"

//Default, can be set through `set_coro_stack_size()`
size_t coro_stack_size = COROUTINE_STACK_SIZE;

// How many unused coroutine stacks to keep around (at most), before they are
// freed. This value is per thread.
const size_t COROUTINE_FREE_LIST_SIZE = 64;

// In debug mode, we print a warning if more than this many coroutines have been
// allocated on one thread.
#ifndef NDEBUG
const int COROS_PER_THREAD_WARN_LEVEL = 10000;
#endif

// The maximum (global) number of stack-protected coroutine stacks.
// On Linux, this number is limited by the setting in `/proc/sys/vm/max_map_count`,
// which is 65536 by default. We use a quarter of that, to leave enough room for the
// memory allocator and other things that might fragment the memory space (note that
// a single protected coroutine consumes two mapped memory regions).
// Exception: debug-mode, where we use a smaller value to exercise the code path more
// often.
#ifdef NDEBUG
const size_t MAX_PROTECTED_COROS = 16384;
#else
const size_t MAX_PROTECTED_COROS = 128;
#endif



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

    /* A list of coroutines that currently have protected stacks. The least recently
    used protected coroutine is always at the front of the list. */
    intrusive_list_t<coro_lru_entry_t> protected_coros_lru;

#ifndef NDEBUG

    /* An integer counting the number of coros on this thread */
    int coro_count;
    /* Have we printed a warning about too many coroutines? We only want to
    print it once. */
    bool printed_high_coro_count_warning;

    /* These variables are used in the implementation of
    `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING`. They record the
    number of things that are currently preventing us from `wait()`ing or
    `notify_now_deprecated()`ing or whatever. */
    int assert_no_coro_waiting_counter;
    std::stack<std::pair<const char *, int> > no_waiting_call_sites;

    int assert_finite_coro_waiting_counter;
    std::stack<std::pair<const char *, int> > finite_waiting_call_sites;

    std::map<std::string, size_t> running_coroutine_counts;
    std::map<std::string, size_t> total_coroutine_counts;

    std::set<coro_t*> active_coroutines;

#endif  // NDEBUG

    coro_globals_t()
        : current_coro(nullptr)
        , prev_coro(nullptr)
#ifndef NDEBUG
        , coro_count(0)
        , printed_high_coro_count_warning(false)
        , assert_no_coro_waiting_counter(0)
        , assert_finite_coro_waiting_counter(0)
#endif
        {
#ifdef _WIN32
            coro_initialize_for_thread();
#endif
        }

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

TLS_with_init(coro_globals_t *, cglobals, nullptr);

// These must be initialized after TLS_cglobals, because perfmon_multi_membership_t
// construction depends on coro_t::coroutines_have_been_initialized() which in turn
// depends on cglobals.
static perfmon_counter_t pm_active_coroutines, pm_allocated_coroutines;
static perfmon_multi_membership_t pm_coroutines_membership(&get_global_perfmon_collection(),
    &pm_active_coroutines, "active_coroutines",
    &pm_allocated_coroutines, "allocated_coroutines");

coro_runtime_t::coro_runtime_t() {
    rassert(!TLS_get_cglobals(), "coro runtime initialized twice on this thread");
    TLS_set_cglobals(new coro_globals_t);
}

coro_runtime_t::~coro_runtime_t() {
    rassert(TLS_get_cglobals());
    delete TLS_get_cglobals();
    TLS_set_cglobals(nullptr);
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
    waiting_(false),
    protected_stack_lru_entry_(this)
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
    if (TLS_get_cglobals()->coro_count > COROS_PER_THREAD_WARN_LEVEL
        && !TLS_get_cglobals()->printed_high_coro_count_warning) {
        TLS_get_cglobals()->printed_high_coro_count_warning = true;
        debugf("A lot of coroutines are allocated on this thread. This could "
               "indicate a misuse of coroutines.\n");
    }
#endif
}

void coro_t::return_coro_to_free_list(coro_t *coro) {
    coro_globals_t *cglobals = TLS_get_cglobals();
    // Note that we must guarantee that `coro` is never evicted immediately. We do so
    // by checking the free list size *before* we push `coro` onto it.
    // This is important because when we call `return_coro_to_free_list` in
    // `coro_t::run`, that coroutine is still active and must not be deleted yet.
    static_assert(COROUTINE_FREE_LIST_SIZE > 0, "COROUTINE_FREE_LIST_SIZE cannot be 0");
    if (cglobals->free_coros.size() >= COROUTINE_FREE_LIST_SIZE) {
        coro_t *coro_to_delete = cglobals->free_coros.tail();
        cglobals->free_coros.remove(coro_to_delete);
        delete coro_to_delete;
    }
    rassert(cglobals->free_coros.size() < COROUTINE_FREE_LIST_SIZE);
    cglobals->free_coros.push_back(coro);
}

coro_t::~coro_t() {
    /* We never move contexts from one thread to another. */
    rassert(get_thread_id() == home_thread());

#ifndef NDEBUG
    TLS_get_cglobals()->coro_count--;
#endif
    --pm_allocated_coroutines;
}

/* Helper function for switching into a new context and making sure that the new context
is protected against stack overflows. Might also unprotect old contexts to avoid
exhausting kernel limits on memory-mapped regions. */
void coro_t::switch_to_coro_with_protection(coro_context_ref_t *current_context) {
    /* Move ourselves to the back of the `protected_coros_lru` list. */
    coro_globals_t *cglobals = TLS_get_cglobals();
    if (protected_stack_lru_entry_.in_a_list()) {
        cglobals->protected_coros_lru.remove(&protected_stack_lru_entry_);
    }
    cglobals->protected_coros_lru.push_back(&protected_stack_lru_entry_);

    /* If there are too many protected coroutines, unprotect the oldest one and pop it of
    the list. */
    size_t max_protected_coros_per_thread =
        MAX_PROTECTED_COROS / static_cast<size_t>(get_num_threads());
    while (cglobals->protected_coros_lru.size() > max_protected_coros_per_thread) {
        cglobals->protected_coros_lru.head()->coro->stack.disable_overflow_protection();
        cglobals->protected_coros_lru.pop_front();
    }

    /* Enable protection for us. (if `max_protected_coros_per_thread` was rounded to 0,
    we exceed the limit here but that's fine. It's important that the currently active
    coroutine is protected.) */
    stack.enable_overflow_protection();

    /* Now actually perform the context switch. */
    context_switch(current_context, &stack.context);
}

void switch_to_scheduler(
        coro_context_ref_t *current_context, coro_context_ref_t *scheduler) {
    context_switch(current_context, scheduler);
}

void coro_t::run() {
    coro_t *coro = TLS_get_cglobals()->current_coro;

#ifndef NDEBUG
    char dummy;
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
        TLS_get_cglobals()->running_coroutine_counts[coro->coroutine_type]++;
        TLS_get_cglobals()->total_coroutine_counts[coro->coroutine_type]++;
        TLS_get_cglobals()->active_coroutines.insert(coro);
#endif
        PROFILER_CORO_RESUME;
        coro->action_wrapper.run();
        PROFILER_CORO_YIELD(0);
#ifndef NDEBUG
        TLS_get_cglobals()->running_coroutine_counts[coro->coroutine_type]--;
        TLS_get_cglobals()->active_coroutines.erase(coro);
#endif

        rassert(coro->current_thread_ == get_thread_id());

        // Destroy the Callable object which was either allocated within the coro_t or on the heap
        coro->action_wrapper.reset();

        coro_globals_t *cglobals_on_final_thread = TLS_get_cglobals();

        /* Remove the coroutine from the `protected_coros_lru` list before we return it
        to the free list. We must do this now rather than in `~coro_t` because
        `~coro_t` is going to run on the home thread of the coroutine, which is not
        necessarily the latest thread on which the coroutine has been executing.
        The `protected_coros_lru` entry (if any) will be on the latest thread where it
        has been executing, so it must be removed there.
        We don't call `disable_stack_protection()` here to increase the efficiency
        of the free list. This means that we can slightly exceed the maximum number of
        protected coroutines (`MAX_PROTECTED_COROS`), by at most
        `COROUTINE_FREE_LIST_SIZE` per thread. */
        if (coro->protected_stack_lru_entry_.in_a_list()) {
            cglobals_on_final_thread->protected_coros_lru.remove(
                &coro->protected_stack_lru_entry_);
        }

        /* Return the context to the free-contexts list we took it from.

        Important: This is ok only because `do_on_thread` and the `linux_message_hub_t`
        are *push* based when delivering messages to another thread. That property
        guarantees that the message is not executed on another thread before we yield to
        the message hub on this one.
        If it was executed immediately on another thread, `coro` could get deleted from
        the free list before we have performed the next context switch, meaning that we
        would free a coroutine that's still running.
        `do_on_thread` does execute `return_coro_to_free_list` immediately if the
        `coro`'s home thread is the current thread. That too is ok though, because the
        implementation of `return_coro_to_free_list` guarantees that `coro` is not going
        to be freed immediately. */
        do_on_thread(coro->home_thread(), std::bind(&coro_t::return_coro_to_free_list, coro));
        --pm_active_coroutines;

        if (cglobals_on_final_thread->prev_coro) {
            cglobals_on_final_thread->prev_coro->switch_to_coro_with_protection(
                &coro->stack.context);
        } else {
            switch_to_scheduler(&coro->stack.context, &cglobals_on_final_thread->scheduler);
        }
    }
}

#ifndef NDEBUG
// This function parses out the type that a coroutine was created with (usually a std::bind), by
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
    // Make a local copy because TLS_get_cglobals() can't be inlined, and we don't
    // want to call it twice.
    coro_globals_t *globals = TLS_get_cglobals();
    return globals == nullptr ? nullptr : globals->current_coro;
}

void coro_t::wait() {   /* class method */
    rassert(self(), "Not in a coroutine context");
    rassert(TLS_get_cglobals()->assert_finite_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d",
        TLS_get_cglobals()->finite_waiting_call_sites.top().first, TLS_get_cglobals()->finite_waiting_call_sites.top().second);

    rassert(TLS_get_cglobals()->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use coro_t::wait().\nConstraint imposed at: %s:%d",
        TLS_get_cglobals()->no_waiting_call_sites.top().first, TLS_get_cglobals()->no_waiting_call_sites.top().second);

    rassert(!self()->waiting_);
    self()->waiting_ = true;

    PROFILER_CORO_YIELD(1);
    if (TLS_get_cglobals()->prev_coro) {
        TLS_get_cglobals()->prev_coro->switch_to_coro_with_protection(
            &self()->stack.context);
    } else {
        switch_to_scheduler(&self()->stack.context, &TLS_get_cglobals()->scheduler);
    }
    PROFILER_CORO_RESUME;

    rassert(self());
    rassert(self()->waiting_);
    self()->waiting_ = false;
}

void coro_t::yield() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify_sometime();
    self()->wait();
}

void coro_t::yield_ordered() {  /* class method */
    rassert(self(), "Not in a coroutine context");
    self()->notify_later_ordered();
    self()->wait();
}

void coro_t::notify_now_deprecated() {
    rassert(waiting_);
    rassert(!notified_);
    rassert(current_thread_.threadnum == linux_thread_pool_t::get_thread_id());

#ifndef NDEBUG
    rassert(TLS_get_cglobals()->assert_no_coro_waiting_counter == 0,
        "This code path is not supposed to use notify_now_deprecated() or spawn_now_dangerously().");

    /* Record old value of `assert_finite_coro_waiting_counter`. It must be legal to call
    `coro_t::wait()` within the coro we are going to jump to, or else we would never jump
    back. */
    int old_assert_finite_coro_waiting_counter = TLS_get_cglobals()->assert_finite_coro_waiting_counter;
    TLS_get_cglobals()->assert_finite_coro_waiting_counter = 0;
#endif

    if (coro_t::self() != nullptr) {
        PROFILER_CORO_YIELD(1);
    }
    coro_t *prev_prev_coro = TLS_get_cglobals()->prev_coro;
    TLS_get_cglobals()->prev_coro = TLS_get_cglobals()->current_coro;
    TLS_get_cglobals()->current_coro = this;

    if (TLS_get_cglobals()->prev_coro) {
        switch_to_coro_with_protection(&TLS_get_cglobals()->prev_coro->stack.context);
    } else {
        switch_to_coro_with_protection(&TLS_get_cglobals()->scheduler);
    }

    rassert(TLS_get_cglobals()->current_coro == this);
    TLS_get_cglobals()->current_coro = TLS_get_cglobals()->prev_coro;
    TLS_get_cglobals()->prev_coro = prev_prev_coro;
    if (coro_t::self() != nullptr) {
        PROFILER_CORO_RESUME;
    }

#ifndef NDEBUG
    /* Restore old value of `assert_finite_coro_waiting_counter`. */
    TLS_get_cglobals()->assert_finite_coro_waiting_counter = old_assert_finite_coro_waiting_counter;
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
    /* Remove ourselves from the `protected_coros_lru` list on the old thread, since
    those lists are not thread-safe.
    We will be added to the one on the new thread once we are woken up there, so there's
    no need to add ourselves now even if we're currently protected.
    Note that this can lead to us temporarily exceeding the protected coro limit if a lot
    of coroutines are in flight between threads at the same time. Hopefully this isn't
    going to be too common. */
    if (self()->protected_stack_lru_entry_.in_a_list()) {
        TLS_get_cglobals()->protected_coros_lru.remove(
            &self()->protected_stack_lru_entry_);
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

/* Called by SIGSEGV/SIGBUS handler to identify segfaults that come from overflowing
a coroutine's stack. Could also in theory be used by a function to check if it's
about to overflow the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    return TLS_get_cglobals()->current_coro && TLS_get_cglobals()->current_coro->stack.address_is_stack_overflow(addr);
}

bool has_n_bytes_free_stack_space(size_t n) {
    // We assume that `tester` is going to be allocated on the stack.
    // Theoretically this is not guaranteed by the C++ standard, but in practice
    // it should work.
    char tester;
    const coro_t *current_coro = coro_t::self();
    guarantee(current_coro != nullptr);
#ifdef _WIN32
    if (!current_coro->stack.has_stack_info) {
        return true;
    }
#endif
    return current_coro->stack.free_space_below(&tester) >= n;
}

bool coroutines_have_been_initialized() {
    return TLS_get_cglobals() != nullptr;
}

coro_t * coro_t::get_coro() {
    rassert(coroutines_have_been_initialized());
    coro_t *coro;

    if (TLS_get_cglobals()->free_coros.size() == 0) {
        coro = new coro_t();
    } else {
        coro = TLS_get_cglobals()->free_coros.tail();
        TLS_get_cglobals()->free_coros.remove(coro);
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
    if (space_remaining > 0 && self() != nullptr) {
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
    guarantee(buffer_out != nullptr);
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
assert_no_coro_waiting_t::assert_no_coro_waiting_t(const char *filename, int line_no) {
    TLS_get_cglobals()->no_waiting_call_sites.push(std::make_pair(filename, line_no));
    TLS_get_cglobals()->assert_no_coro_waiting_counter++;
}
assert_no_coro_waiting_t::~assert_no_coro_waiting_t() {
    TLS_get_cglobals()->no_waiting_call_sites.pop();
    TLS_get_cglobals()->assert_no_coro_waiting_counter--;
}
assert_finite_coro_waiting_t::assert_finite_coro_waiting_t(const char *filename, int line_no) {
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
