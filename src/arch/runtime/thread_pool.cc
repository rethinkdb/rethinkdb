#include "arch/runtime/thread_pool.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "arch/runtime/event_queue.hpp"
#include "arch/runtime/runtime.hpp"
#include "errors.hpp"
#include "logger.hpp"

const int SEGV_STACK_SIZE = 8126;

__thread linux_thread_pool_t *linux_thread_pool_t::thread_pool;
__thread int linux_thread_pool_t::thread_id;
__thread linux_thread_t *linux_thread_pool_t::thread;

linux_thread_pool_t::linux_thread_pool_t(int worker_threads, bool _do_set_affinity) :
#ifndef NDEBUG
      coroutine_summary(false),
#endif
      interrupt_message(NULL),
      generic_blocker_pool(NULL),
      n_threads(worker_threads + 1),    // we create an extra utility thread
      do_set_affinity(_do_set_affinity)
{
    rassert(n_threads > 1);             // we want at least one non-utility thread
    rassert(n_threads <= MAX_THREADS);

    int res;

    res = pthread_cond_init(&shutdown_cond, NULL);
    guarantee(res == 0, "Could not create shutdown cond");

    res = pthread_mutex_init(&shutdown_cond_mutex, NULL);
    guarantee(res == 0, "Could not create shutdown cond mutex");

    res = pthread_spin_init(&interrupt_message_lock, PTHREAD_PROCESS_PRIVATE);
    guarantee(res == 0, "Could not create interrupt spin lock");
}

linux_thread_message_t *linux_thread_pool_t::set_interrupt_message(linux_thread_message_t *m) {
    int res;

    res = pthread_spin_lock(&thread_pool->interrupt_message_lock);
    guarantee(res == 0, "Could not acquire interrupt message lock");

    linux_thread_message_t *o = thread_pool->interrupt_message;
    thread_pool->interrupt_message = m;

    res = pthread_spin_unlock(&thread_pool->interrupt_message_lock);
    guarantee(res == 0, "Could not release interrupt message lock");

    return o;
}

struct thread_data_t {
    pthread_barrier_t *barrier;
    linux_thread_pool_t *thread_pool;
    int current_thread;
    linux_thread_message_t *initial_message;
};

void *linux_thread_pool_t::start_thread(void *arg) {
    // Block all signals but `SIGSEGV` (will be unblocked by the event queue in
    // case of poll).
    {
        sigset_t sigmask;
        int res = sigfillset(&sigmask);
        guarantee_err(res == 0, "Could not get a full sigmask");

        res = sigdelset(&sigmask, SIGSEGV);
        guarantee_err(res == 0, "Could not remove SIGSEGV from sigmask");

        res = pthread_sigmask(SIG_SETMASK, &sigmask, NULL);
        guarantee_err(res == 0, "Could not block signal");
    }

    thread_data_t *tdata = reinterpret_cast<thread_data_t *>(arg);

    // Set thread-local variables
    linux_thread_pool_t::thread_pool = tdata->thread_pool;
    linux_thread_pool_t::thread_id = tdata->current_thread;

    // Use a separate block so that it's very clear how long the thread lives for
    // It's not really necessary, but I like it.
    {
        linux_thread_t thread(tdata->thread_pool, tdata->current_thread);
        tdata->thread_pool->threads[tdata->current_thread] = &thread;
        linux_thread_pool_t::thread = &thread;
        blocker_pool_t *generic_blocker_pool = NULL; // Will only be instantiated by one thread

        /* Install a handler for segmentation faults that just prints a backtrace. If we're
        running under valgrind, we don't install this handler because Valgrind will print the
        backtrace for us. */
#ifndef VALGRIND
        stack_t segv_stack;
        segv_stack.ss_sp = malloc_aligned(SEGV_STACK_SIZE, getpagesize());
        guarantee_err(segv_stack.ss_sp != 0, "malloc failed");
        segv_stack.ss_flags = 0;
        segv_stack.ss_size = SEGV_STACK_SIZE;
        int r = sigaltstack(&segv_stack, NULL);
        guarantee_err(r == 0, "sigaltstack failed");

        struct sigaction action;
        bzero(&action, sizeof(action));
        action.sa_flags = SA_SIGINFO | SA_STACK;
        action.sa_sigaction = &linux_thread_pool_t::sigsegv_handler;
        r = sigaction(SIGSEGV, &action, NULL);
        guarantee_err(r == 0, "Could not install SEGV handler");
#endif

        // First thread should initialize generic_blocker_pool before the start barrier
        if (tdata->initial_message) {
            rassert(tdata->thread_pool->generic_blocker_pool == NULL, "generic_blocker_pool already initialized");
            generic_blocker_pool = new blocker_pool_t(GENERIC_BLOCKER_THREAD_COUNT,
                                                      &thread.queue);
            tdata->thread_pool->generic_blocker_pool = generic_blocker_pool;
        }

        // If one thread is allowed to run before another one has finished
        // starting up, then it might try to access an uninitialized part of the
        // unstarted one.
        int res = pthread_barrier_wait(tdata->barrier);
        guarantee(res == 0 || res == PTHREAD_BARRIER_SERIAL_THREAD, "Could not wait at start barrier");
        rassert(tdata->thread_pool->generic_blocker_pool != NULL,
                "Thread passed start barrier while generic_blocker_pool uninitialized");

        // Prime the pump by calling the initial thread message that was passed to thread_pool::run()
        if (tdata->initial_message) {
            thread.message_hub.store_message(tdata->current_thread, tdata->initial_message);
        }

        thread.queue.run();

        // If one thread is allowed to delete itself before another one has
        // broken out of its loop, it might delete something that the other thread
        // needed to access.
        res = pthread_barrier_wait(tdata->barrier);
        guarantee(res == 0 || res == PTHREAD_BARRIER_SERIAL_THREAD, "Could not wait at stop barrier");

#ifndef VALGRIND
        free(segv_stack.ss_sp);
#endif

        // If this thread created the generic blocker pool, clean it up
        if (generic_blocker_pool != NULL) {
            delete generic_blocker_pool;
            tdata->thread_pool->generic_blocker_pool = NULL;
        }

        tdata->thread_pool->threads[tdata->current_thread] = NULL;
        linux_thread_pool_t::thread = NULL;
    }

    delete tdata;
    return NULL;
}

#ifndef NDEBUG
void linux_thread_pool_t::enable_coroutine_summary() {
    coroutine_summary = true;
}
#endif

void linux_thread_pool_t::run_thread_pool(linux_thread_message_t *initial_message) {
    int res;

    do_shutdown = false;

    // Start child threads
    pthread_barrier_t barrier;
    res = pthread_barrier_init(&barrier, NULL, n_threads);
    guarantee(res == 0, "Could not create barrier");

    for (int i = 0; i < n_threads; ++i) {
        thread_data_t *tdata = new thread_data_t();
        tdata->barrier = &barrier;
        tdata->thread_pool = this;
        tdata->current_thread = i;
        // The initial message gets sent to thread zero.
        tdata->initial_message = (i == 0) ? initial_message : NULL;

        res = pthread_create(&pthreads[i], NULL, &start_thread, tdata);
        guarantee(res == 0, "Could not create thread");

        if (do_set_affinity) {
            // Distribute threads evenly among CPUs
            int ncpus = get_cpu_count();
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(i % ncpus, &mask);
            res = pthread_setaffinity_np(pthreads[i], sizeof(cpu_set_t), &mask);
            guarantee(res == 0, "Could not set thread affinity");
        }
    }

    // Mark the main thread (for use in assertions etc.)
    linux_thread_pool_t::thread_id = -1;

    // Set up interrupt handlers

    // TODO: Should we save and restore previous interrupt handlers? This would
    // be a good thing to do before distributing the RethinkDB IO layer, but it's
    // not really important.

    linux_thread_pool_t::thread_pool = this;   // So signal handlers can find us
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = &linux_thread_pool_t::interrupt_handler;

        res = sigaction(SIGTERM, &sa, NULL);
        guarantee_err(res == 0, "Could not install TERM handler");

        res = sigaction(SIGINT, &sa, NULL);
        guarantee_err(res == 0, "Could not install INT handler");
    }

    // Wait for order to shut down

    res = pthread_mutex_lock(&shutdown_cond_mutex);
    guarantee(res == 0, "Could not lock shutdown cond mutex");

    while (!do_shutdown) {   // while loop guards against spurious wakeups
        res = pthread_cond_wait(&shutdown_cond, &shutdown_cond_mutex);
        guarantee(res == 0, "Could not wait for shutdown cond");
    }

    res = pthread_mutex_unlock(&shutdown_cond_mutex);
    guarantee(res == 0, "Could not unlock shutdown cond mutex");

    // Remove interrupt handlers

    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = SIG_IGN;

        res = sigaction(SIGTERM, &sa, NULL);
        guarantee_err(res == 0, "Could not remove TERM handler");

        res = sigaction(SIGINT, &sa, NULL);
        guarantee_err(res == 0, "Could not remove INT handler");
    }
    linux_thread_pool_t::thread_pool = NULL;

#ifndef NDEBUG
    // Save each thread's coroutine counters before shutting down
    std::vector<std::map<std::string, size_t> > coroutine_counts(n_threads);
#endif

    // Shut down child threads
    for (int i = 0; i < n_threads; ++i) {
        // Cause child thread to break out of its loop
#ifndef NDEBUG
        threads[i]->initiate_shut_down(&coroutine_counts[i]);
#else
        threads[i]->initiate_shut_down();
#endif
    }

    for (int i = 0; i < n_threads; ++i) {
        // Wait for child thread to actually exit

        res = pthread_join(pthreads[i], NULL);
        guarantee(res == 0, "Could not join thread");
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
            logDBG("%ld coroutines ran with type %s", i->second, i->first.c_str());
        }
    }
#endif

    res = pthread_barrier_destroy(&barrier);
    guarantee(res == 0, "Could not destroy barrier");
}

// Note: Maybe we should use a signalfd instead of a signal handler, and then
// there would be no issues with potential race conditions because the signal
// would just be pulled out in the main poll/epoll loop. But as long as this works,
// there's no real reason to change it.
void linux_thread_pool_t::interrupt_handler(int) {
    /* The interrupt handler should run on the main thread, the same thread that
    run() was called on. */
    rassert(linux_thread_pool_t::thread_id == -1, "The interrupt handler was called on the wrong thread.");

    linux_thread_pool_t *self = linux_thread_pool_t::thread_pool;

    /* Set the interrupt message to NULL at the same time as we get it so that
    we don't send the same message twice. This is necessary because it's illegal
    to send the same thread message twice until it has been received the first time
    (because of the intrusive list), and we could hypothetically get two SIGINTs
    in quick succession. */
    linux_thread_message_t *interrupt_msg = self->set_interrupt_message(NULL);

    if (interrupt_msg) {
        self->threads[self->n_threads - 1]->message_hub.insert_external_message(interrupt_msg);
    }
}

void linux_thread_pool_t::sigsegv_handler(int signum, siginfo_t *info, UNUSED void *data) {
    if (signum == SIGSEGV) {
        if (is_coroutine_stack_overflow(info->si_addr)) {
            crash("Callstack overflow in a coroutine");
        } else {
            crash("Segmentation fault from reading the address %p.", info->si_addr);
        }
    } else {
        crash("Unexpected signal: %d\n", signum);
    }
}

void linux_thread_pool_t::shutdown_thread_pool() {
    int res;

    // This will tell the main() thread to tell all the child threads to
    // shut down.

    res = pthread_mutex_lock(&shutdown_cond_mutex);
    guarantee(res == 0, "Could not lock shutdown cond mutex");

    do_shutdown = true;

    res = pthread_cond_signal(&shutdown_cond);
    guarantee(res == 0, "Could not signal shutdown cond");

    res = pthread_mutex_unlock(&shutdown_cond_mutex);
    guarantee(res == 0, "Could not unlock shutdown cond mutex");
}

linux_thread_pool_t::~linux_thread_pool_t() {
    int res;

    res = pthread_cond_destroy(&shutdown_cond);
    guarantee(res == 0, "Could not destroy shutdown cond");

    res = pthread_mutex_destroy(&shutdown_cond_mutex);
    guarantee(res == 0, "Could not destroy shutdown cond mutex");

    res = pthread_spin_destroy(&interrupt_message_lock);
    guarantee(res == 0, "Could not destroy interrupt spin lock");
}

linux_thread_t::linux_thread_t(linux_thread_pool_t *parent_pool, int thread_id)
    : queue(this),
      message_hub(&queue, parent_pool, thread_id),
      timer_handler(&queue),
      do_shutdown(false)
#ifndef NDEBUG
      , coroutine_counts_at_shutdown(NULL)
#endif
{
    // Initialize the mutex which synchronizes access to the do_shutdown variable
    pthread_mutex_init(&do_shutdown_mutex, NULL);

    // Watch an eventfd for shutdown notifications
    queue.watch_resource(shutdown_notify_event.get_notify_fd(), poll_event_in, this);
}

linux_thread_t::~linux_thread_t() {

#ifndef NDEBUG
    // Save the coroutine counts before they're deleted, should be ready at shutdown
    rassert(coroutine_counts_at_shutdown != NULL);
    coroutine_counts_at_shutdown->clear();
    coro_runtime.get_coroutine_counts(coroutine_counts_at_shutdown);
#endif

    guarantee(pthread_mutex_destroy(&do_shutdown_mutex) == 0);
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
    pthread_mutex_lock(&do_shutdown_mutex);
    bool result = do_shutdown;
    pthread_mutex_unlock(&do_shutdown_mutex);
    return result;
}

#ifndef NDEBUG
void linux_thread_t::initiate_shut_down(std::map<std::string, size_t> *coroutine_counts) {
#else
void linux_thread_t::initiate_shut_down() {
#endif
    pthread_mutex_lock(&do_shutdown_mutex);
#ifndef NDEBUG
    coroutine_counts_at_shutdown = coroutine_counts;
#endif
    do_shutdown = true;
    shutdown_notify_event.write(1);
    pthread_mutex_unlock(&do_shutdown_mutex);
}
