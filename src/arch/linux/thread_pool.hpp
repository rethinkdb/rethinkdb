#ifndef __LINUX_THREAD_POOL_HPP__
#define __LINUX_THREAD_POOL_HPP__

#include <pthread.h>
#include "config/args.hpp"

struct linux_cpu_message_t;
struct linux_event_queue_t;

/* A thread pool represents a group of threads, each of which is associated with an
event queue. There is one thread pool per server. It is responsible for starting up
and shutting down the threads and event queues. */

class linux_thread_pool_t {

public:
    linux_thread_pool_t(int n_threads);
    
    // When the process receives a SIGINT or SIGTERM, interrupt_message will be delivered to the
    // same thread that initial_message was delivered to, and interrupt_message will be set to
    // NULL. If you want to receive notification of further SIGINTs or SIGTERMs, you must call
    // set_interrupt_message() again. Returns the previous value of interrupt_message.
    linux_cpu_message_t *set_interrupt_message(linux_cpu_message_t *interrupt_message);
    
    // Blocks while threads are working. Only returns after shutdown() is called. initial_message
    // is a CPU message that will be delivered to one of the threads after all of the event queues
    // have been started; it is used to start the server's activity.
    void run(linux_cpu_message_t *initial_message);
    
    // Shut down all the threads. Can be called from any thread.
    void shutdown();
    
    ~linux_thread_pool_t();

private:
    static void *start_event_queue(void*);
    
    static void interrupt_handler(int);
    pthread_spinlock_t interrupt_message_lock;
    linux_cpu_message_t *interrupt_message;
    
    // Used to signal the main thread for shutdown
    bool do_shutdown;
    pthread_cond_t shutdown_cond;
    pthread_mutex_t shutdown_cond_mutex;

public:
    pthread_t threads[MAX_CPUS];
    linux_event_queue_t *queues[MAX_CPUS];
    
    int n_threads;
    // The thread_pool that started the thread we are currently in
    static __thread linux_thread_pool_t *thread_pool;
    // The ID of the thread we are currently in
    static __thread int cpu_id;
    // The event queue for the thread we are currently in (same as thread_pool->queues[cpu_id])
    static __thread linux_event_queue_t *event_queue;
};

#endif /* __LINUX_THREAD_POOL_HPP__ */
