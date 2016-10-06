// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/thread_pool.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "arch/compiler.hpp"
#include "arch/barrier.hpp"
#include "arch/os_signal.hpp"
#include "arch/io/timer_provider.hpp"
#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/timing.hpp"
#include "errors.hpp"
#include "logger.hpp"
#include "utils.hpp"

#if !defined(VALGRIND) && !defined(_WIN32)
// This needs to be large enough for abi::__cxa_demangle to not overflow the stack.
// If this is too small, you may see memory corruption and crashes when attempting
// to format a backtrace.
const int SIGNAL_HANDLER_STACK_SIZE = MINSIGSTKSZ + (128 * KILOBYTE);
#endif

#ifdef _WIN32
std::atomic<linux_thread_pool_t *> linux_thread_pool_t::global_thread_pool = nullptr;

linux_thread_pool_t *linux_thread_pool_t::get_global_thread_pool() {
    return global_thread_pool.load();
}
#endif

THREAD_LOCAL linux_thread_pool_t *linux_thread_pool_t::thread_pool = nullptr;
THREAD_LOCAL int linux_thread_pool_t::thread_id = -1;
THREAD_LOCAL linux_thread_t *linux_thread_pool_t::thread = nullptr;

linux_thread_pool_t *linux_thread_pool_t::get_thread_pool() {
    return thread_pool;
}
void linux_thread_pool_t::set_thread_pool(linux_thread_pool_t *val) {
    rassert(thread_pool == nullptr || val == nullptr);
    thread_pool = val;
}
int linux_thread_pool_t::get_thread_id() {
    return thread_id;
}
void linux_thread_pool_t::set_thread_id(int val) {
    thread_id = val;
}
linux_thread_t *linux_thread_pool_t::get_thread() {
    rassert(thread != nullptr);
    return thread;
}
void linux_thread_pool_t::set_thread(linux_thread_t *val) {
    rassert(thread == nullptr || val == nullptr);
    thread = val;
}

linux_thread_pool_t::linux_thread_pool_t(int worker_threads, bool _do_set_affinity) :
#ifndef NDEBUG
      coroutine_summary(false),
#endif
      interrupt_message(nullptr),
      generic_blocker_pool(nullptr),
      n_threads(worker_threads + 1),    // we create an extra utility thread
      do_set_affinity(_do_set_affinity)
{
    rassert(n_threads > 1);             // we want at least one non-utility thread
    rassert(n_threads <= MAX_THREADS);

    int res;

    res = pthread_cond_init(&shutdown_cond, nullptr);
    guarantee_xerr(res == 0, res, "Could not create shutdown cond");

    res = pthread_mutex_init(&shutdown_cond_mutex, nullptr);
    guarantee_xerr(res == 0, res, "Could not create shutdown cond mutex");
}

os_signal_cond_t *linux_thread_pool_t::exchange_interrupt_message(os_signal_cond_t *m) {
    os_signal_cond_t *o;
    {
        spinlock_acq_t acq(&interrupt_message_lock);

        o = interrupt_message;

        interrupt_message = m;
    }

    return o;
}

struct thread_data_t {
    thread_barrier_t *barrier;
    linux_thread_pool_t *thread_pool;
    int current_thread;
    linux_thread_message_t *initial_message;
};

void *linux_thread_pool_t::start_thread(void *arg) {
#ifndef _WIN32
    // Block all signals but `SIGSEGV` and `SIGBUS` (will be unblocked by the event
    // queue in case of poll).
    {
        sigset_t sigmask;
        int res = sigfillset(&sigmask);
        guarantee_err(res == 0, "Could not get a full sigmask");

        res = sigdelset(&sigmask, SIGSEGV);
        guarantee_err(res == 0, "Could not remove SIGSEGV from sigmask");
        res = sigdelset(&sigmask, SIGBUS);
        guarantee_err(res == 0, "Could not remove SIGBUS from sigmask");

        res = pthread_sigmask(SIG_SETMASK, &sigmask, nullptr);
        guarantee_xerr(res == 0, res, "Could not block signal");
    }
#endif

    thread_data_t *tdata = reinterpret_cast<thread_data_t *>(arg);

    // Set thread-local variables
    set_thread_pool(tdata->thread_pool);
    set_thread_id(tdata->current_thread);

    // Use a separate block so that it's very clear how long the thread lives for
    // It's not really necessary, but I like it.
    {
        linux_thread_t local_thread(tdata->thread_pool, tdata->current_thread);
        tdata->thread_pool->threads[tdata->current_thread] = &local_thread;
        set_thread(&local_thread);
        blocker_pool_t *generic_blocker_pool = nullptr; // Will only be instantiated by one thread

        /* Install a handler for segmentation faults that just prints a backtrace. If we're
        running under valgrind, we don't install this handler because Valgrind will print the
        backtrace for us. */
#ifndef VALGRIND
#ifndef _WIN32
        scoped_page_aligned_ptr_t<void> stack_base(SIGNAL_HANDLER_STACK_SIZE);
        stack_t signal_stack;
        signal_stack.ss_sp = stack_base.get();
        signal_stack.ss_flags = 0;
        signal_stack.ss_size = SIGNAL_HANDLER_STACK_SIZE;
        int res = sigaltstack(&signal_stack, nullptr);
        guarantee_err(res == 0, "sigaltstack failed");

        {
            struct sigaction sa = make_sa_sigaction(
                SA_SIGINFO | SA_ONSTACK,
                &linux_thread_pool_t::fatal_signal_handler);

            res = sigaction(SIGSEGV, &sa, nullptr);
            guarantee_err(res == 0, "Could not install SEGV signal handler");
            res = sigaction(SIGBUS, &sa, nullptr);
            guarantee_err(res == 0, "Could not install BUS signal handler");
        }
#endif
#endif  // VALGRIND

        // First thread should initialize generic_blocker_pool before the start barrier
        if (tdata->initial_message) {
            rassert(tdata->thread_pool->generic_blocker_pool == nullptr, "generic_blocker_pool already initialized");
            generic_blocker_pool = new blocker_pool_t(GENERIC_BLOCKER_THREAD_COUNT,
                                                      &local_thread.queue);
            tdata->thread_pool->generic_blocker_pool = generic_blocker_pool;
        }

        // If one thread is allowed to run before another one has finished
        // starting up, then it might try to access an uninitialized part of the
        // unstarted one.
        tdata->barrier->wait();
        rassert(tdata->thread_pool->generic_blocker_pool != nullptr,
                "Thread passed start barrier while generic_blocker_pool uninitialized");

        // Prime the pump by calling the initial thread message that was passed to thread_pool::run()
        if (tdata->initial_message) {
            local_thread.message_hub.insert_external_message(tdata->initial_message);
        }

        local_thread.queue.run();

        // If one thread is allowed to delete itself before another one has
        // broken out of its loop, it might delete something that the other thread
        // needed to access.
        tdata->barrier->wait();

        // If this thread created the generic blocker pool, clean it up
        if (generic_blocker_pool != nullptr) {
            delete generic_blocker_pool;
            tdata->thread_pool->generic_blocker_pool = nullptr;
        }

        tdata->thread_pool->threads[tdata->current_thread] = nullptr;
        set_thread(nullptr);
    }

    delete tdata;
    return nullptr;
}

#ifndef NDEBUG
void linux_thread_pool_t::enable_coroutine_summary() {
    coroutine_summary = true;
}
#endif

void linux_thread_pool_t::run_thread_pool(linux_thread_message_t *initial_message) {
    do_shutdown = false;

    // Start child threads
    thread_barrier_t barrier(n_threads + 1);

    for (int i = 0; i < n_threads; i++) {
        bool is_utility_thread = (i == n_threads - 1);
        thread_data_t *tdata = new thread_data_t();
        tdata->barrier = &barrier;
        tdata->thread_pool = this;
        tdata->current_thread = i;
        // The initial message gets sent to the utility thread.
        tdata->initial_message = is_utility_thread ? initial_message : nullptr;

        int res = pthread_create(&pthreads[i], nullptr, &start_thread, tdata);
        guarantee_xerr(res == 0, res, "Could not create thread");

        // Don't set affinity for the utility thread
        if (do_set_affinity && !is_utility_thread) {
            // On Apple, the thread affinity API has awful documentation, so we don't even bother.
#ifdef _GNU_SOURCE
            // Distribute threads evenly among CPUs
            int ncpus = get_cpu_count();
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(i % ncpus, &mask);
            res = pthread_setaffinity_np(pthreads[i], sizeof(cpu_set_t), &mask);
            guarantee_xerr(res == 0, res, "Could not set thread affinity");
#endif
        }
    }

    // Mark the main thread (for use in assertions etc.)
    set_thread_id(-1);

    // Set up interrupt handlers

    // Wait for threads to start up so that our interrupt handlers can send messages to the threads.
    // TODO: Fix the thread pool so that it isn't awful, fragile, and reliant on thread barriers.
    barrier.wait();

    // TODO: Should we save and restore previous interrupt handlers? This would
    // be a good thing to do before distributing the RethinkDB IO layer, but it's
    // not really important.

    set_thread_pool(this);   // So signal handlers can find us
#ifdef _WIN32
    global_thread_pool.store(this);
#else
    {
        struct sigaction sa = make_sa_sigaction(SA_SIGINFO, &linux_thread_pool_t::interrupt_handler);

        int res = sigaction(SIGTERM, &sa, nullptr);
        guarantee_err(res == 0, "Could not install TERM handler");

        res = sigaction(SIGINT, &sa, nullptr);
        guarantee_err(res == 0, "Could not install INT handler");
    }
#endif

    // Wait for order to shut down

    int res = pthread_mutex_lock(&shutdown_cond_mutex);
    guarantee_xerr(res == 0, res, "Could not lock shutdown cond mutex");

    while (!do_shutdown) {   // while loop guards against spurious wakeups
        res = pthread_cond_wait(&shutdown_cond, &shutdown_cond_mutex);
        guarantee_xerr(res == 0, res, "Could not wait for shutdown cond");
    }

    res = pthread_mutex_unlock(&shutdown_cond_mutex);
    guarantee_xerr(res == 0, res, "Could not unlock shutdown cond mutex");

#ifdef _WIN32
    global_thread_pool.store(nullptr);
#else
    // Remove interrupt handlers
    {
        struct sigaction sa = make_sa_handler(0, SIG_IGN);

        res = sigaction(SIGTERM, &sa, nullptr);
        guarantee_err(res == 0, "Could not remove TERM handler");

        res = sigaction(SIGINT, &sa, nullptr);
        guarantee_err(res == 0, "Could not remove INT handler");
    }
#endif
    set_thread_pool(nullptr);

#ifndef NDEBUG
    // Save each thread's coroutine counters before shutting down
    std::vector<std::map<std::string, size_t> > coroutine_counts(n_threads);
#endif

    // Shut down child threads
    for (int i = 0; i < n_threads; i++) {
        // Cause child thread to break out of its loop
#ifndef NDEBUG
        threads[i]->initiate_shut_down(&coroutine_counts[i]);
#else
        threads[i]->initiate_shut_down();
#endif
    }

    // Wait for barrier, because it expects n_threads + 1 things to wait.  (Otherwise we'd have to
    // have two barriers, which isn't such a problem, but meh.)
    barrier.wait();

    for (int i = 0; i < n_threads; i++) {
        // Wait for child thread to actually exit

        res = pthread_join(pthreads[i], nullptr);
        guarantee_xerr(res == 0, res, "Could not join thread");
    }

#ifndef NDEBUG
    if (coroutine_summary)
    {
        // Combine coroutine counts from each thread, and log the totals
        std::map<std::string, size_t> total_coroutine_counts;
        for (int i = 0; i < n_threads; ++i) {
            for (std::map<std::string, size_t>::iterator j = coroutine_counts[i].begin();
                 j != coroutine_counts[i].end(); ++j) {
                total_coroutine_counts[j->first] += j->second;
            }
        }

        for (std::map<std::string, size_t>::iterator i = total_coroutine_counts.begin();
             i != total_coroutine_counts.end(); ++i) {
            logDBG("%zu coroutines ran with type %s", i->second, i->first.c_str());
        }
    }
#endif  // NDEBUG
}

#ifdef _WIN32

void linux_thread_pool_t::interrupt_handler(DWORD type) {
    // The  handler should run on a new thread created by the OS
    rassert(get_thread_pool() == nullptr, "The interrupt handler was called on the wrong thread.");

    linux_thread_pool_t *self = global_thread_pool.load();

    if (self == nullptr) {
        logNTC("Caught signal. Waiting for initialization to complete before closing...");

        const int WAIT_FOR_THREAD_POOL_MS = 10000;
        const int NAP_TIME = 200;

        for (int napped = 0; ; napped += NAP_TIME) {
            Sleep(NAP_TIME);
            self = global_thread_pool.load();
            if (self != nullptr) {
                break;
            }
            if (napped >= WAIT_FOR_THREAD_POOL_MS) {
                logNTC("Waiting for initilization timed out. Stopping RethinkDB.");
                ExitProcess(EXIT_FAILURE);
            }
        }
    }

    os_signal_cond_t *interrupt_signal = self->exchange_interrupt_message(nullptr);

    if (interrupt_signal != nullptr) {
        interrupt_signal->source_type = type;
        self->threads[self->n_threads - 1]->message_hub.insert_external_message(interrupt_signal);
    }
}

#else

// Note: Maybe we should use a signalfd instead of a signal handler, and then
// there would be no issues with potential race conditions because the signal
// would just be pulled out in the main poll/epoll loop. But as long as this works,
// there's no real reason to change it.
void linux_thread_pool_t::interrupt_handler(int signo, siginfo_t *siginfo, void *) {
    /* The interrupt handler should run on the main thread, the same thread that
    run() was called on. */
    rassert(get_thread_id() == -1, "The interrupt handler was called on the wrong thread.");

    linux_thread_pool_t *self = get_thread_pool();

    /* Set the interrupt message to NULL at the same time as we get it so that
    we don't send the same message twice. This is necessary because it's illegal
    to send the same thread message twice until it has been received the first time
    (because of the intrusive list), and we could hypothetically get two SIGINTs
    in quick succession. */
    os_signal_cond_t *interrupt_signal = self->exchange_interrupt_message(NULL);
    if (interrupt_signal != NULL) {
        interrupt_signal->source_signo = signo;
        interrupt_signal->source_pid = siginfo->si_pid;
        interrupt_signal->source_uid = siginfo->si_uid;
        self->threads[self->n_threads - 1]->message_hub.insert_external_message(interrupt_signal);
    }
}

void linux_thread_pool_t::fatal_signal_handler(
        int signum,
        siginfo_t *info,
        UNUSED void *data) {
    // Linux generates a SIGSEGV when accessing a protected page, OS X generates
    // SIGBUS.
    if ((signum == SIGSEGV || signum == SIGBUS)
        && is_coroutine_stack_overflow(info->si_addr)) {
        crash("Callstack overflow in a coroutine");
    } else if (signum == SIGSEGV) {
        crash("Segmentation fault from reading the address %p.", info->si_addr);
    } else if (signum == SIGBUS) {
        crash("Bus error from accessing the address %p.", info->si_addr);
    } else {
        crash("Unexpected signal: %d\n", signum);
    }
}
#endif

void linux_thread_pool_t::shutdown_thread_pool() {
    int res;

    // This will tell the main() thread to tell all the child threads to
    // shut down.

    res = pthread_mutex_lock(&shutdown_cond_mutex);
    guarantee_xerr(res == 0, res, "Could not lock shutdown cond mutex");

    do_shutdown = true;

    res = pthread_cond_signal(&shutdown_cond);
    guarantee_xerr(res == 0, res, "Could not signal shutdown cond");

    res = pthread_mutex_unlock(&shutdown_cond_mutex);
    guarantee_xerr(res == 0, res, "Could not unlock shutdown cond mutex");
}

linux_thread_pool_t::~linux_thread_pool_t() {
    int res;

    res = pthread_cond_destroy(&shutdown_cond);
    guarantee_xerr(res == 0, res, "Could not destroy shutdown cond");

    res = pthread_mutex_destroy(&shutdown_cond_mutex);
    guarantee_xerr(res == 0, res, "Could not destroy shutdown cond mutex");
}

linux_thread_t::linux_thread_t(linux_thread_pool_t *parent_pool, int thread_id)
    : queue(this),
      message_hub(&queue, parent_pool, threadnum_t(thread_id)),
      timer_handler(&queue),
      do_shutdown(false)
#ifndef NDEBUG
      , coroutine_counts_at_shutdown(NULL)
#endif
{
    // Initialize the mutex which synchronizes access to the do_shutdown variable
    int res = pthread_mutex_init(&do_shutdown_mutex, nullptr);
    guarantee_xerr(res == 0, res, "could not initialize do_shutdown_mutex");

    // Watch an eventfd for shutdown notifications
    queue.watch_event(&shutdown_notify_event, this);
}

linux_thread_t::~linux_thread_t() {

#ifndef NDEBUG
    // Save the coroutine counts before they're deleted, should be ready at shutdown
    rassert(coroutine_counts_at_shutdown != nullptr);
    coroutine_counts_at_shutdown->clear();
    coro_runtime.get_coroutine_counts(coroutine_counts_at_shutdown);
#endif

    int res = pthread_mutex_destroy(&do_shutdown_mutex);
    guarantee_xerr(res == 0, res, "could not destroy do_shutdown_mutex");
}

void linux_thread_t::pump() {
    message_hub.push_messages();
}

void linux_thread_t::on_event(int events) {
    // No-op. This is just to make sure that the event queue wakes up
    // so it can shut down.

    if (events != poll_event_in) {
        logERR("Unexpected event mask: %d", events);
    }
}

bool linux_thread_t::should_shut_down() {
    int res = pthread_mutex_lock(&do_shutdown_mutex);
    guarantee_xerr(res == 0, res, "could not lock do_shutdown_mutex");
    bool result = do_shutdown;
    res = pthread_mutex_unlock(&do_shutdown_mutex);
    guarantee_xerr(res == 0, res, "could not unlock do_shutdown_mutex");
    return result;
}

#ifndef NDEBUG
void linux_thread_t::initiate_shut_down(std::map<std::string, size_t> *coroutine_counts) {
#else
void linux_thread_t::initiate_shut_down() {
#endif
    int res = pthread_mutex_lock(&do_shutdown_mutex);
    guarantee_xerr(res == 0, res, "could not lock do_shutdown_mutex");
#ifndef NDEBUG
    coroutine_counts_at_shutdown = coroutine_counts;
#endif
    do_shutdown = true;
    shutdown_notify_event.wakey_wakey();
    res = pthread_mutex_unlock(&do_shutdown_mutex);
    guarantee_xerr(res == 0, res, "could not unlock do_shutdown_mutex");
}
