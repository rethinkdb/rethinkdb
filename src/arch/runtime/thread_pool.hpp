// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef ARCH_RUNTIME_THREAD_POOL_HPP_
#define ARCH_RUNTIME_THREAD_POOL_HPP_

#include <pthread.h>

#include <map>
#include <string>
#include <atomic>

#include "arch/compiler.hpp"
#include "config/args.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/system_event.hpp"
#include "arch/runtime/message_hub.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/io/blocker_pool.hpp"
#include "arch/io/timer_provider.hpp"
#include "arch/timer.hpp"

class linux_thread_t;
class os_signal_cond_t;

/* coro_runtime_t is borrowed from coroutines.hpp.  Please only
construct one coro_runtime_t per thread. Coroutines can only be used
when a coro_runtime_t exists. It exists to take advantage of RAII. */

struct coro_runtime_t {
    coro_runtime_t();
    ~coro_runtime_t();

#ifndef NDEBUG
    void get_coroutine_counts(std::map<std::string, size_t> *dest);
#endif
};


/* A thread pool represents a group of threads, each of which is associated with an
event queue. There is one thread pool per server. It is responsible for starting up
and shutting down the threads and event queues. */

class linux_thread_pool_t {
public:
    linux_thread_pool_t(int worker_threads, bool do_set_affinity);

    // When the process receives a SIGINT or SIGTERM, interrupt_message will be delivered to the
    // same thread that initial_message was delivered to, and interrupt_message will be set to
    // NULL. If you want to receive notification of further SIGINTs or SIGTERMs, you must call
    // exchange_interrupt_message() again. Returns the previous value of interrupt_message.
    MUST_USE os_signal_cond_t *exchange_interrupt_message(os_signal_cond_t *interrupt_message);

    // Blocks while threads are working. Only returns after shutdown() is called. initial_message
    // is a thread message that will be delivered to thread zero after all of the event queues
    // have been started; it is used to start the server's activity.
    void run_thread_pool(linux_thread_message_t *initial_message);

#ifndef NDEBUG
    void enable_coroutine_summary();
#endif

    // Shut down all the threads. Can be called from any thread.
    void shutdown_thread_pool();

    ~linux_thread_pool_t();

#ifdef _WIN32
    static void interrupt_handler(DWORD type);
#endif

private:
#ifndef NDEBUG
    bool coroutine_summary;
#endif

    static void *start_thread(void*);

#ifndef _WIN32
    static void interrupt_handler(int signo, siginfo_t *siginfo, void *);
    // Currently handles SIGSEGV and SIGBUS signals.
    static void fatal_signal_handler(int, siginfo_t *, void *) NORETURN;
#endif

    spinlock_t interrupt_message_lock;
    os_signal_cond_t *interrupt_message;

    // Used to signal the main thread for shutdown
    volatile bool do_shutdown;
    pthread_cond_t shutdown_cond;
    pthread_mutex_t shutdown_cond_mutex;

    // The number of threads to allocate for handling blocking calls
    static const int GENERIC_BLOCKER_THREAD_COUNT = 2;
    blocker_pool_t* generic_blocker_pool;

public:
    pthread_t pthreads[MAX_THREADS];
    linux_thread_t *threads[MAX_THREADS];

    // Cooperatively run a blocking function call using the generic_blocker_pool
    template <class Callable>
    static void run_in_blocker_pool(const Callable &);

    int n_threads;
    bool do_set_affinity;

#ifdef _WIN32
    static linux_thread_pool_t *get_global_thread_pool();
#endif

    // Getters and setters for the thread local variables.
    // See thread_local.hpp for an explanation of why these must not be
    // inlined.
    static NOINLINE linux_thread_pool_t *get_thread_pool();
    static NOINLINE void set_thread_pool(linux_thread_pool_t *val);
    static NOINLINE int get_thread_id();
    static NOINLINE void set_thread_id(int val);
    static NOINLINE linux_thread_t *get_thread();
    static NOINLINE void set_thread(linux_thread_t *val);

private:

#ifdef _WIN32
    static std::atomic<linux_thread_pool_t *> global_thread_pool;
#endif

    // The thread_pool that started the thread we are currently in
    static THREAD_LOCAL linux_thread_pool_t *thread_pool;
    // The ID of the thread we are currently in
    static THREAD_LOCAL int thread_id;
    // The event queue for the thread we are currently in (same as &thread_pool->threads[thread_id])
    static THREAD_LOCAL linux_thread_t *thread;

    DISABLE_COPYING(linux_thread_pool_t);
};

template <class Callable>
struct generic_job_t :
    public blocker_pool_t::job_t
{
    void run() {
        (*fn)();
    }

    void done() {
        // Now that the function is done, resume execution of the suspended task
        suspended->notify_sometime();
    }

    const Callable *fn;
    coro_t* suspended;
};

// Function to handle blocking calls in a separate thread pool
// This should be used for any calls that cannot otherwise be made non-blocking
template <class Callable>
void linux_thread_pool_t::run_in_blocker_pool(const Callable &fn)
{
    if (get_thread_pool() != nullptr) {
        generic_job_t<Callable> job;
        job.fn = &fn;
        job.suspended = coro_t::self();

        rassert(get_thread_pool()->generic_blocker_pool != NULL,
                "thread_pool_t::run_in_blocker_pool called while generic_thread_pool uninitialized");
        get_thread_pool()->generic_blocker_pool->do_job(&job);

        // Give up execution, to be resumed when the done callback is made
        coro_t::wait();
    } else {
        // Thread pool has not been created, just block the current thread, since we won't be
        //  screwing up any coroutines
        fn();
    }
}

class linux_thread_t :
    public linux_event_callback_t,
    public linux_queue_parent_t {
public:
    linux_thread_t(linux_thread_pool_t *parent_pool, int thread_id);
    ~linux_thread_t();

    linux_event_queue_t queue;
    linux_message_hub_t message_hub;
    timer_handler_t timer_handler;

    /* Never accessed; its constructor and destructor set up and tear down thread-local variables
    for coroutines. */
    coro_runtime_t coro_runtime;

    void pump();   // Called by the event queue
    bool should_shut_down();   // Called by the event queue
#ifndef NDEBUG
    void initiate_shut_down(std::map<std::string, size_t> *coroutine_counts); // Can be called from any thread
#else
    void initiate_shut_down(); // Can be called from any thread
#endif
    void on_event(int events);

private:
    volatile bool do_shutdown;
    pthread_mutex_t do_shutdown_mutex;
    system_event_t shutdown_notify_event;

#ifndef NDEBUG
    std::map<std::string, size_t> *coroutine_counts_at_shutdown;
#endif
};

#endif /* ARCH_RUNTIME_THREAD_POOL_HPP_ */
