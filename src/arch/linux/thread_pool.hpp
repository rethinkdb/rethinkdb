#ifndef __LINUX_THREAD_POOL_HPP__
#define __LINUX_THREAD_POOL_HPP__

#include <pthread.h>
#include "config/args.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/disk.hpp"
#include "arch/linux/message_hub.hpp"
#include "arch/linux/coroutines.hpp"
#include "arch/timer.hpp"

struct linux_thread_message_t;
struct linux_thread_t;

/* A thread pool represents a group of threads, each of which is associated with an
event queue. There is one thread pool per server. It is responsible for starting up
and shutting down the threads and event queues. */

class linux_thread_pool_t {
public:
    explicit linux_thread_pool_t(int n_threads);
    
    // When the process receives a SIGINT or SIGTERM, interrupt_message will be delivered to the
    // same thread that initial_message was delivered to, and interrupt_message will be set to
    // NULL. If you want to receive notification of further SIGINTs or SIGTERMs, you must call
    // set_interrupt_message() again. Returns the previous value of interrupt_message.
    static linux_thread_message_t *set_interrupt_message(linux_thread_message_t *interrupt_message);
    
    // Blocks while threads are working. Only returns after shutdown() is called. initial_message
    // is a thread message that will be delivered to one of the threads after all of the event queues
    // have been started; it is used to start the server's activity.
    void run(linux_thread_message_t *initial_message);
    
    // Shut down all the threads. Can be called from any thread.
    void shutdown();
    
    ~linux_thread_pool_t();

private:
    static void *start_thread(void*);
    
    static void interrupt_handler(int);
    static void sigsegv_handler(int, siginfo_t *, void *);
    pthread_spinlock_t interrupt_message_lock;
    linux_thread_message_t *interrupt_message;
    
    // Used to signal the main thread for shutdown
    volatile bool do_shutdown;
    pthread_cond_t shutdown_cond;
    pthread_mutex_t shutdown_cond_mutex;

public:
    pthread_t pthreads[MAX_THREADS];
    linux_thread_t *threads[MAX_THREADS];
    
    int n_threads;
    // The thread_pool that started the thread we are currently in
    static __thread linux_thread_pool_t *thread_pool;
    // The ID of the thread we are currently in
    static __thread int thread_id;
    // The event queue for the thread we are currently in (same as &thread_pool->threads[thread_id])
    static __thread linux_thread_t *thread;
};

class linux_thread_t :
    public linux_event_callback_t,
    public linux_queue_parent_t
{
    timer_token_t *perfmon_stats_timer;
    
public:
    linux_thread_t(linux_thread_pool_t *parent_pool, int thread_id);
    ~linux_thread_t();
    
    linux_event_queue_t queue;
    linux_message_hub_t message_hub;
    timer_handler_t timer_handler;
    linux_io_calls_t iosys;
    
    /* Never accessed; its constructor and destructor set up and tear down thread-local variables
    for coroutines. */
    coro_globals_t coro_globals;
    
    volatile bool do_shutdown;
    fd_t shutdown_notify_fd;
    void pump();   // Called by the event queue
    bool should_shut_down();   // Called by the event queue
    void on_event(int events);
};

#endif /* __LINUX_THREAD_POOL_HPP__ */
