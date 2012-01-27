#ifndef __ARCH_LINUX_COROUTINES_HPP__
#define __ARCH_LINUX_COROUTINES_HPP__

#include "utils2.hpp"
#include <list>
#include <vector>
#include <boost/bind.hpp>
#include "arch/linux/message_hub.hpp"
#include "containers/intrusive_list.hpp"

int get_thread_id();

const size_t MAX_COROUTINE_STACK_SIZE = 8*1024*1024;

/* Please only construct one coro_globals_t per thread. Coroutines can only be used when
a coro_globals_t exists. It exists to take advantage of RAII. */

struct coro_globals_t {
    coro_globals_t();
    ~coro_globals_t();
};

#define CALLABLE_CUTOFF_SIZE 128

class callable_action_t {
public:
    virtual void run_action() = 0;
    callable_action_t() { }
    virtual ~callable_action_t() { }
private:
    DISABLE_COPYING(callable_action_t);
};

template<class Callable>
class callable_action_instance_t : public callable_action_t {
public:
    callable_action_instance_t(const Callable& callable) : callable_(callable) { }

    void run_action() { callable_(); }

private:
    Callable callable_;
};

class callable_action_wrapper_t {
public:
    callable_action_wrapper_t();
    ~callable_action_wrapper_t();

    template<class Callable>
    void reset(const Callable& action)
    {
        rassert(action_ == NULL);

        // Allocate the action inside this object, if possible, or the heap otherwise
        if (sizeof(Callable) > CALLABLE_CUTOFF_SIZE) {
            action_ = new callable_action_instance_t<Callable>(action);
            action_on_heap = true;
        } else {
            action_ = new (action_data) callable_action_instance_t<Callable>(action);
            action_on_heap = false;
        }
    }

    void reset();

    void run();

private:
    bool action_on_heap;
    callable_action_t *action_;
    char action_data[CALLABLE_CUTOFF_SIZE] __attribute__((aligned(sizeof(void*))));

    DISABLE_COPYING(callable_action_wrapper_t);
};

typedef void *lw_ucontext_t; /* A stack pointer into a stack that has all the other context registers. */

/* A coro_t represents a fiber of execution within a thread. Create one with spawn_*(). Within a
coroutine, call wait() to return control to the scheduler; the coroutine will be resumed when
another fiber calls notify_*() on it.

coro_t objects can switch threads with move_to_thread(), but it is recommended that you use
on_thread_t for more safety. */

struct coro_t : private linux_thread_message_t, public intrusive_list_node_t<coro_t> {
    friend bool is_coroutine_stack_overflow(void *);

public:
    template<class Callable>
    static void spawn_later(const Callable &action) {
        get_and_init_coro(action, get_thread_id())->notify_later();
    }

    template<class Callable>
    static void spawn_now(const Callable &action) {
        get_and_init_coro(action, get_thread_id())->notify_now();
    }

    template<class Callable>
    static void spawn_on_thread(int thread, const Callable &action) {
        get_and_init_coro(action, thread)->notify_later();
    }

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
    // Friend coro_globals, so it can destroy coro_t
    friend class coro_globals_t;

    coro_t();
    static void run();
    ~coro_t();

    virtual void on_thread_switch();

    int home_thread_;
    int current_thread_;

    template<class Callable>
    static coro_t * get_and_init_coro(const Callable &action, int thread) {
        coro_t *coro = get_coro(thread);
        coro->action_wrapper.reset(action);
        return coro;
    }

    static coro_t * get_coro(int thread);
    static void return_coro_to_free_coros(coro_t *coro);

    // Sanity check variables
    bool notified_;
    bool waiting_;
    callable_action_wrapper_t action_wrapper;

    /* The guts of the coro_t */
    void *stack;
    lw_ucontext_t env; // A pointer into the stack.
#ifdef VALGRIND
    unsigned int valgrind_stack_id;
#endif

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

#endif // __ARCH_LINUX_COROUTINES_HPP__
