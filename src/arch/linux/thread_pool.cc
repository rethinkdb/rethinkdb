#include <errno.h>
#include "thread_pool.hpp"
#include "errors.hpp"
#include "arch/linux/event_queue.hpp"
#include <sys/eventfd.h>
#include <signal.h>

__thread linux_thread_pool_t *linux_thread_pool_t::thread_pool;
__thread linux_event_queue_t *linux_thread_pool_t::event_queue;
__thread int linux_thread_pool_t::cpu_id;

linux_thread_pool_t::linux_thread_pool_t(int n_threads)
    : interrupt_message(NULL), n_threads(n_threads)
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

void linux_thread_pool_t::set_interrupt_message(linux_cpu_message_t *m) {
    
    int res;
    
    res = pthread_spin_lock(&interrupt_message_lock);
    check("Could not acquire interrutp message lock", res != 0);
    
    interrupt_message = m;
    
    res = pthread_spin_unlock(&interrupt_message_lock);
    check("Could not release interrupt message lock", res != 0);
}

struct thread_data_t {
    pthread_barrier_t *barrier;
    linux_thread_pool_t *thread_pool;
    int current_cpu;
    linux_cpu_message_t *initial_message;
};

void *linux_thread_pool_t::start_event_queue(void *arg) {
    
    int res;
    
    thread_data_t *tdata = (thread_data_t*)arg;
    
    // Set thread-local variables
    linux_thread_pool_t::thread_pool = tdata->thread_pool;
    linux_thread_pool_t::cpu_id = tdata->current_cpu;
    
    // Use a separate block so that it's very clear how long the event queue lives for
    // It's not really necessary, but I like it.
    {
        linux_event_queue_t event_queue(tdata->thread_pool, tdata->current_cpu);
        tdata->thread_pool->queues[tdata->current_cpu] = &event_queue;
        linux_thread_pool_t::event_queue = &event_queue;
        
        // If one event queue is allowed to run before another one has finished
        // starting up, then it might try to access an uninitialized part of the
        // unstarted one.
        res = pthread_barrier_wait(tdata->barrier);
        check("Could not wait at start barrier", res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD);
        
        // Prime the pump by calling the initial CPU message that was passed to thread_pool::run()
        if (tdata->initial_message) {
            event_queue.message_hub.store_message(tdata->current_cpu, tdata->initial_message);
        }
        
        event_queue.run();
        
        // If one event queue is allowed to delete itself before another one has
        // broken out of its loop, it might delete something that the other thread
        // needed to access.
        res = pthread_barrier_wait(tdata->barrier);
        check("Could not wait at stop barrier", res != 0 && res != PTHREAD_BARRIER_SERIAL_THREAD);
        
        tdata->thread_pool->queues[tdata->current_cpu] = NULL;
        linux_thread_pool_t::event_queue = NULL;
    }
    
    // Delete all custom allocators
    tls_small_obj_alloc_accessor<alloc_t>::alloc_vector_t *allocs =
        tls_small_obj_alloc_accessor<alloc_t>::allocs_tl;
    if (allocs) {
        for (unsigned i = 0; i < allocs->size(); i++) {
            gdelete((*allocs)[i]);
        }
        gdelete(allocs);
    }
    
    gdelete(tdata);
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
        
        thread_data_t *tdata = gnew<thread_data_t>();
        tdata->barrier = &barrier;
        tdata->thread_pool = this;
        tdata->current_cpu = i;
        tdata->initial_message = (i == 0) ? initial_message : NULL;
        
        res = pthread_create(&threads[i], NULL, &start_event_queue, (void*)tdata);
        check("Could not create thread", res != 0);
        
        // Distribute threads evenly among CPUs
        
        int ncpus = get_cpu_count();
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(i % ncpus, &mask);
        res = pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &mask);
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
        
        queues[i]->do_shut_down = true;
        res = eventfd_write(queues[i]->core_notify_fd, 1);
        check("Could not write to core_notify_fd", res != 0);
    }
    
    for (int i = 0; i < n_threads; i++) {
    
        // Wait for child thread to actually exit
        
        res = pthread_join(threads[i], NULL);
        check("Could not join thread", res != 0);
    }
    
    res = pthread_barrier_destroy(&barrier);
    check("Could not destroy barrier", res != 0);
    
    // Fin.
}

void linux_thread_pool_t::interrupt_handler(int) {
    
    int res;
    
    linux_thread_pool_t *self = linux_thread_pool_t::thread_pool;
    
    linux_cpu_message_t *interrupt_msg;
    
    res = pthread_spin_lock(&self->interrupt_message_lock);
    check("Could not acquire interrupt message lock", res != 0);
    
    interrupt_msg = self->interrupt_message;
    self->interrupt_message = NULL;
    
    res = pthread_spin_unlock(&self->interrupt_message_lock);
    check("Could not release interrupt message lock", res != 0);
    
    if (interrupt_msg) {
        self->queues[0]->message_hub.insert_external_message(interrupt_msg);
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
