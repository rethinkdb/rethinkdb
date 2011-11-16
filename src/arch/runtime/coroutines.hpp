#ifndef __ARCH_RUNTIME_COROUTINES_HPP__
#define __ARCH_RUNTIME_COROUTINES_HPP__

#ifndef NDEBUG
#include <string>
#endif

#include "errors.hpp"
#include <boost/function.hpp>

#include "arch/runtime/runtime_utils.hpp"

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

class coro_context_t;
struct artificial_stack_t;

/* A coro_t represents a fiber of execution within a thread. Create one with spawn_*(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify_*() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

class coro_t : private linux_thread_message_t {
public:
    friend class coro_context_t;
    friend bool is_coroutine_stack_overflow(void *);

    static void spawn_now(const boost::function<void()> &deed);
    static void spawn_sometime(const boost::function<void()> &deed);

    // TODO: spawn_later_ordered is usually what naive people want,
    // but it's such a long and onerous name.  It should have the
    // shortest name.
    static void spawn_later_ordered(const boost::function<void()> &deed);
    static void spawn_on_thread(int thread, const boost::function<void()> &deed);

    // Use coro_t::spawn_*(boost::bind(...)) for spawning with parameters.

public:
    /* Pauses the current coroutine until it is notified */
    static void wait();

    /* Gives another coroutine a chance to run, but schedules this coroutine to
    be run at some point in the future. Might not preserve order; two calls to
    `yield()` by different coroutines may return in a different order than they
    began in. */
    static void yield();

    /* Returns a pointer to the current coroutine, or `NULL` if we are not in a
    coroutine. */
    static coro_t *self();

    /* Transfers control immediately to the coroutine. Returns when the
    coroutine calls `wait()`.

    Note: `notify_now()` may become deprecated eventually. The original purpose
    was to provide better performance than could be achieved with
    `notify_later_ordered()`, but `notify_sometime()` is now filling that role
    instead. */
    void notify_now();

    /* Schedules the coroutine to be woken up eventually. Can be safely called
    from any thread. Returns immediately. Does not provide any ordering
    guarantees. If you don't need the ordering guarantees that
    `notify_later_ordered()` provides, use `notify_sometime()` instead. */
    void notify_sometime();

    // TODO: notify_later_ordered is usually what naive people want
    // and should get, but it's such a long and onerous name.  It
    // should have the shortest name.

    /* Pushes the coroutine onto the event queue for the thread it's currently
    on, such that it will be run. This can safely be called from any thread.
    Returns immediately. If you call `notify_later_ordered()` on two coroutines
    that are on the same thread, they will run in the same order you call
    `notify_later_ordered()` in. */
    void notify_later_ordered();

#ifndef NDEBUG
    // A unique identifier for this particular instance of coro_t over
    // the lifetime of the process.
    static int64_t selfname() {
        coro_t *self = coro_t::self();
        return self ? self->selfname_number : 0;
    }

    int64_t selfname_number;
#endif

public:
    /* `spawn()` and `notify()` are aliases for `spawn_later_ordered()` and
    `notify_later_ordered()`. They are deprecated and new code should not use
    them. */
    static void spawn(const boost::function<void()> &deed) {
        spawn_later_ordered(deed);
    }

public:
    static void set_coroutine_stack_size(size_t size);

    artificial_stack_t* get_stack();

private:
    /* When called from within a coroutine, schedules the coroutine to be run on
    the given thread and then suspends the coroutine until that other thread
    picks it up again. Do not call this directly; use `on_thread_t` instead. */
    friend class on_thread_t;
    static void move_to_thread(int thread);

    /* Internally, we recycle ucontexts and stacks. So the real guts of the coroutine are in the
    coro_context_t object. */
    coro_context_t *context;

    coro_t(const boost::function<void()>& deed);
    boost::function<void()> deed_;
    void run();
    ~coro_t();

    virtual void on_thread_switch();

    int current_thread_;

    // Sanity check variables
    bool notified_;
    bool waiting_;

    DISABLE_COPYING(coro_t);
};

/* Returns true if the given address is in the protection page of the current coroutine. */
bool is_coroutine_stack_overflow(void *addr);

#ifndef NDEBUG

/* If `ASSERT_NO_CORO_WAITING;` appears at the top of a block, then it is illegal
to call `coro_t::wait()`, `coro_t::spawn_now()`, or `coro_t::notify_now()`
within that block and any attempt to do so will be a fatal error. */
#define ASSERT_NO_CORO_WAITING assert_no_coro_waiting_t assert_no_coro_waiting_var(__FILE__, __LINE__)

/* If `ASSERT_FINITE_CORO_WAITING;` appears at the top of a block, then code
within that block may call `coro_t::spawn_now()` or `coro_t::notify_now()` but
not `coro_t::wait()`. This is because `coro_t::spawn_now()` and
`coro_t::notify_now()` will return control directly to the coroutine that called
then. */
#define ASSERT_FINITE_CORO_WAITING assert_finite_coro_waiting_t assert_finite_coro_waiting_var(__FILE__, __LINE__)

/* Implementation support for `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING` */
struct assert_no_coro_waiting_t {
    assert_no_coro_waiting_t(std::string, int);
    ~assert_no_coro_waiting_t();
};
struct assert_finite_coro_waiting_t {
    assert_finite_coro_waiting_t(std::string, int);
    ~assert_finite_coro_waiting_t();
};

#else  // NDEBUG

/* In release mode, these assertions are no-ops. */
#define ASSERT_NO_CORO_WAITING do { } while (0)
#define ASSERT_FINITE_CORO_WAITING do { } while (0)

#endif  // NDEBUG

#endif // __ARCH_RUNTIME_COROUTINES_HPP__
