#ifndef __ARCH_RUNTIME_COROUTINES_HPP__
#define __ARCH_RUNTIME_COROUTINES_HPP__

#include "errors.hpp"
#include <boost/function.hpp>
#include "arch/runtime/linux_utils.hpp"

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

class coro_context_t;

/* A coro_t represents a fiber of execution within a thread. Create one with spawn_*(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify_*() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

class coro_t : private linux_thread_message_t {
public:
    friend class coro_context_t;
    friend bool is_coroutine_stack_overflow(void *);

    static void spawn_later(const boost::function<void()> &deed);
    static void spawn_now(const boost::function<void()> &deed);
    static void spawn_on_thread(int thread, const boost::function<void()> &deed);

    // Use coro_t::spawn_*(boost::bind(...)) for spawning with parameters.

public:
    static void wait();         // Pauses the current coroutine until it's notified
    static void yield();        // Pushes the current coroutine to the end of the notify queue and waits
    static coro_t *self();      // Returns the current coroutine
    void notify_now();          // Switches to a coroutine immediately (will switch back when it returns or wait()s)
    void notify_later();        // Wakes up the coroutine, allowing the scheduler to trigger it to continue
    static void move_to_thread(int thread); // Wait and notify self on the CPU (avoiding race conditions)

    // `spawn()` and `notify()` are synonyms for `spawn_later()` and `notify_later()`; their
    // use is discouraged because they are ambiguous.
    static void spawn(const boost::function<void()> &deed) {
        spawn_later(deed);
    }
    void notify() {
        notify_later();
    }

public:
    static void set_coroutine_stack_size(size_t size);

private:
    /* Internally, we recycle ucontexts and stacks. So the real guts of the coroutine are in the
    coro_context_t object. */
    coro_context_t *context;

    coro_t(const boost::function<void()>& deed, int thread);
    boost::function<void()> deed_;
    void run();
    ~coro_t();

    virtual void on_thread_switch();

    int current_thread_;
    int original_free_contexts_thread_;

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
#define ASSERT_NO_CORO_WAITING assert_no_coro_waiting_t assert_no_coro_waiting_var

/* If `ASSERT_FINITE_CORO_WAITING;` appears at the top of a block, then code
within that block may call `coro_t::spawn_now()` or `coro_t::notify_now()` but
not `coro_t::wait()`. This is because `coro_t::spawn_now()` and
`coro_t::notify_now()` will return control directly to the coroutine that called
then. */
#define ASSERT_FINITE_CORO_WAITING assert_finite_coro_waiting_t assert_finite_coro_waiting_var

/* Implementation support for `ASSERT_NO_CORO_WAITING` and `ASSERT_FINITE_CORO_WAITING` */
struct assert_no_coro_waiting_t {
    assert_no_coro_waiting_t();
    ~assert_no_coro_waiting_t();
};
struct assert_finite_coro_waiting_t {
    assert_finite_coro_waiting_t();
    ~assert_finite_coro_waiting_t();
};

#else  // NDEBUG

/* In release mode, these assertions are no-ops. */
#define ASSERT_NO_CORO_WAITING do { } while (0)
#define ASSERT_FINITE_CORO_WAITING do { } while (0)

#endif  // NDEBUG

#endif // __ARCH_RUNTIME_COROUTINES_HPP__
