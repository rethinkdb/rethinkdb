#include <errno.h>
#include <unistd.h>
#include "thread_pool.hpp"
#include "errors.hpp"
#include "arch/linux/event_queue.hpp"
#include "coroutine/coroutines.hpp"
#include <sys/eventfd.h>
#include <signal.h>
#include <fcntl.h>

/* Defined in perfmon_system.cc */
void poll_system_stats(void *);

__thread linux_thread_pool_t *linux_thread_pool_t::thread_pool;
__thread int linux_thread_pool_t::cpu_id;
__thread linux_thread_t *linux_thread_pool_t::thread;

linux_thread_pool_t::linux_thread_pool_t(int n_threads)
    : interrupt_message(NULL),
      n_threads(n_threads + 1) // we create an extra utility thread
{
    assert(n_threads > 0);
    assert(n_threads <= MAX_CPUS);
    
    int res;
    
    res = pthread_cond_init(&shutdown_cond, NULL);
    check("Could not create shutdown cond", res != 0);
    
    res = pthread_mutex_init(&shutdown_cond_mutex, NULL);
    check("Could not create shutdown cond mutex", res != 0);
    
    res = pthread_spin_init(&interrupt_message_lock, PTHREAD_PROCESS_PRIVATE);
    check("Could not create interrupt spin lock", res != 0);
}

linux_cpu_message_t *linux_thread_pool_t::set_interrupt_message(linux_cpu_message_t *m) {
    
    int res;
    
    res = pthread_spin_lock(&interrupt_message_lock);
    check("Could not acquire interrutp message lock", res != 0);
    
    linux_cpu_message_t *o = interrupt_message;
    interrupt_message = m;
    
    res = pthread_spin_unlock(&interrupt_message_lock);
    check("Could not release interrupt message lock", res != 0);
    
    return o;
}

struct thread_data_t {
    pthread_barrier_t *barrier;
    linux_thread_pool_t *thread_pool;
    int current_cpu;
    linux_cpu_message_t *initial_message;
};

void *linux_thread_pool_t::start_thread(void *arg) {
    
    int res;
    
    thread_data_t *tdata = (thread_data_t*)arg;
    
    // Set thread-local variables
    linux_thread_pool_t::thread_pool = tdata->thread_pool;
    linux_thread_pool_t::cpu_id = tdata->current_cpu;
    
    // Use a separate block so that it's very clear how long the thread lives for
    // It's not really necessary, but I like it.
    {
        linux_thread_t thread(tdata->thread_pool, tdata->current_cpu);
        tdata->thread_pool->threads[tdata->current_cpu] = &thread;
        linux_thread_pool_t::thread = &thread;
        
        // If one thread is allowed to run before another one has finished
        // starting up, then it might try to access an uninitialized part of the
        // unstarted one.
        res = pthread_barrier_wait(tdata->barrier);
        check("Could not wait at start barrier", res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD);
        
        // Prime the pump by calling the initial CPU message that was passed to thread_pool::run()
        if (tdata->initial_message) {
            thread.message_hub.store_message(tdata->current_cpu, tdata->initial_message);
        }

        coro_t::run();
        
        thread.queue.run();
        
        // If one thread is allowed to delete itself before another one has
        // broken out of its loop, it might delete something that the other thread
        // needed to access.
        res = pthread_barrier_wait(tdata->barrier);
        check("Could not wait at stop barrier", res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD);
        
        coro_t::destroy();
        tdata->thread_pool->threads[tdata->current_cpu] = NULL;
        linux_thread_pool_t::thread = NULL;
    }
    
    delete tdata;
    return NULL;
}

void linux_thread_pool_t::run(linux_cpu_message_t *initial_message) {
    
    int res;
    
    do_shutdown = false;
    
    // Start child threads
    
    pthread_barrier_t barrier;
    res = pthread_barrier_init(&barrier, NULL, n_threads);
    check("Could not create barrier", res != 0);
    
    for (int i = 0; i < n_threads; i++) {
        
        thread_data_t *tdata = new thread_data_t();
        tdata->barrier = &barrier;
        tdata->thread_pool = this;
        tdata->current_cpu = i;
        // The initial message (which creates utility workers) gets
        // sent to the last CPU. The last CPU is not reported by
        // get_num_db_cpus() (see it for more details).
        tdata->initial_message = (i == n_threads - 1) ? initial_message : NULL;
        
        res = pthread_create(&pthreads[i], NULL, &start_thread, (void*)tdata);
        check("Could not create thread", res != 0);
        
        // Distribute threads evenly among CPUs
        
        int ncpus = get_cpu_count();
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(i % ncpus, &mask);
        res = pthread_setaffinity_np(pthreads[i], sizeof(cpu_set_t), &mask);
        check("Could not set thread affinity", res != 0);
    }
    
    // Set up interrupt handlers
    
    linux_thread_pool_t::thread_pool = this;   // So signal handlers can find us
    {
        struct sigaction sa;
        bzero((char*)&sa, sizeof(struct sigaction));
        sa.sa_handler = &linux_thread_pool_t::interrupt_handler;
    
        res = sigaction(SIGTERM, &sa, NULL);
        check("Could not install TERM handler", res != 0);
        
        res = sigaction(SIGINT, &sa, NULL);
        check("Could not install INT handler", res != 0);
    }
    
    // Wait for order to shut down
    
    res = pthread_mutex_lock(&shutdown_cond_mutex);
    check("Could not lock shutdown cond mutex", res != 0);
    
    while (!do_shutdown) {   // while loop guards against spurious wakeups
    
        res = pthread_cond_wait(&shutdown_cond, &shutdown_cond_mutex);
        check("Could not wait for shutdown cond", res != 0);
    }
    
    res = pthread_mutex_unlock(&shutdown_cond_mutex);
    check("Could not unlock shutdown cond mutex", res != 0);
    
    // Remove interrupt handlers
    
    {
        struct sigaction sa;
        bzero((char*)&sa, sizeof(struct sigaction));
        sa.sa_handler = SIG_DFL;
    
        res = sigaction(SIGTERM, &sa, NULL);
        check("Could not remove TERM handler", res != 0);
        
        res = sigaction(SIGINT, &sa, NULL);
        check("Could not remove INT handler", res != 0);
    }
    linux_thread_pool_t::thread_pool = NULL;
    
    // Shut down child threads
    
    for (int i = 0; i < n_threads; i++) {
        
        // Cause child thread to break out of its loop
        
        threads[i]->do_shutdown = true;
        res = eventfd_write(threads[i]->shutdown_notify_fd, 1);
        check("Could not write to core_notify_fd", res != 0);
    }
    
    for (int i = 0; i < n_threads; i++) {
    
        // Wait for child thread to actually exit
        
        res = pthread_join(pthreads[i], NULL);
        check("Could not join thread", res != 0);
    }
    
    res = pthread_barrier_destroy(&barrier);
    check("Could not destroy barrier", res != 0);
    
    // Fin.
}

// Note: Maybe we should use a signalfd instead of a signal handler, and then
// there would be no issues with potential race conditions because the signal
// would just be pulled out in the main poll/epoll loop. But as long as this works,
// there's no real reason to change it.
void linux_thread_pool_t::interrupt_handler(int) {
    
    /* The interrupt handler should run on the main thread, the same thread that
    run() was called on. */
    
    linux_thread_pool_t *self = linux_thread_pool_t::thread_pool;
    
    /* Set the interrupt message to NULL at the same time as we get it so that
    we don't send the same message twice. This is necessary because it's illegal
    to send the same CPU message twice until it has been received the first time
    (because of the intrusive list), and we could hypothetically get two SIGINTs
    in quick succession. */
    linux_cpu_message_t *interrupt_msg = self->set_interrupt_message(NULL);
    
    if (interrupt_msg) {
        self->threads[self->n_threads - 1]->message_hub.insert_external_message(interrupt_msg);
    }
}

void linux_thread_pool_t::shutdown() {
    
    int res;
    
    // This will tell the main thread to tell all the child threads to
    // shut down.
    
    do_shutdown = true;
    
    res = pthread_cond_signal(&shutdown_cond);
    check("Could not signal shutdown cond", res != 0);
}

linux_thread_pool_t::~linux_thread_pool_t() {
    
    int res;
    
    res = pthread_cond_destroy(&shutdown_cond);
    check("Could not destroy shutdown cond", res != 0);
    
    res = pthread_mutex_destroy(&shutdown_cond_mutex);
    check("Could not destroy shutdown cond mutex", res != 0);
    
    res = pthread_spin_destroy(&interrupt_message_lock);
    check("Could not destroy interrupt spin lock", res != 0);
}

linux_thread_t::linux_thread_t(linux_thread_pool_t *parent_pool, int thread_id)
    : queue(this),
      message_hub(&queue, parent_pool, thread_id),
      timer_handler(&queue),
      iosys(&queue),
      do_shutdown(false)
{
    int res;
    
    // Create and watch an eventfd for shutdown notifications
    
    shutdown_notify_fd = eventfd(0, 0);
    check("Could not create shutdown notification fd", shutdown_notify_fd == -1);

    res = fcntl(shutdown_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make shutdown notify fd non-blocking", res != 0);

    queue.watch_resource(shutdown_notify_fd, poll_event_in, this);
    
    // Start the stats timer
    
    perfmon_stats_timer = timer_handler.add_timer_internal(1000, poll_system_stats, NULL, false);
}

linux_thread_t::~linux_thread_t() {
    
    timer_handler.cancel_timer(perfmon_stats_timer);
    
    int res;
    
    res = close(shutdown_notify_fd);
    check("Could not close shutdown_notify_fd", res != 0);
}

void linux_thread_t::pump() {
    
    message_hub.push_messages();
}

void linux_thread_t::on_event(int events) {
    
    // No-op. This is just to make sure that the event queue wakes up
    // so it can shut down.
}

bool linux_thread_t::should_shut_down() {

    return do_shutdown;
}

