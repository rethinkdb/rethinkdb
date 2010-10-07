
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <pthread.h>
#include <libaio.h>
#include <queue>
#include <sys/epoll.h>
#include "corefwd.hpp"
#include "message_hub.hpp"

struct linux_epoll_callback_t {
    virtual void on_epoll(int events) = 0;
};

typedef int fd_t;
#define INVALID_FD fd_t(-1)

typedef fd_t resource_t;

/* TODO: Merge linux_io_calls_t with linux_event_queue_t */

class linux_io_calls_t {
    
    friend class linux_event_queue_t;
    friend class linux_direct_file_t;
    
    linux_io_calls_t(linux_event_queue_t *_queue);
    virtual ~linux_io_calls_t();

    typedef std::vector<iocb*, gnew_alloc<iocb*> > request_vector_t;

    void process_requests();
    int process_request_batch(request_vector_t *requests);

    linux_event_queue_t *queue;

    request_vector_t r_requests, w_requests;

    int n_pending;

#ifndef NDEBUG
    // We need the extra pointer in debug mode because
    // tls_small_obj_alloc_accessor creates the pools to expect it
    static const size_t iocb_size = sizeof(iocb) + sizeof(void*);
#else
    static const size_t iocb_size = sizeof(iocb);
#endif

public:
    /**
     * AIO notification support (this is meant to be called by the
     * event queue).
     */
    void aio_notify(iocb *event, int result);
};

// Event queue structure
struct linux_event_queue_t {
public:
    linux_event_queue_t(linux_thread_pool_t *thread_pool, int current_cpu);
    void run();
    ~linux_event_queue_t();
    
public:
    struct timer_t : public intrusive_list_node_t<timer_t>,
                     public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, timer_t> {
        
        friend class linux_event_queue_t;
        
    private:
        bool once;   // If 'false', the timer is repeating
        long interval_ms;   // If a repeating timer, this is the time between 'rings'
        long next_time_in_ms;   // This is the time (in ms since the server started) of the next 'ring'
        
        // It's unsafe to remove arbitrary timers from the list as we iterate over
        // it, so instead we set the 'deleted' flag and then remove them in a
        // controlled fashion.
        bool deleted;
        
        void (*callback)(void *ctx);
        void *context;
    };

public:
    // TODO: be clear on what should and shouldn't be public here
    io_context_t aio_context;
    fd_t aio_notify_fd, core_notify_fd;
    fd_t timer_fd;
    long timer_ticks_since_server_startup;
    fd_t epoll_fd;
    linux_message_hub_t message_hub;
    bool do_shut_down;   // Main thread sets this to true and then signals core_notify_fd. This makes the event queue shut down.
    linux_io_calls_t iosys;
    // We store this as a class member because forget_resource needs
    // to go through the events and remove queued messages for
    // resources that are being destroyed.
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    int nevents;

private:
    void process_timer_notify();
    void process_aio_notify();
    static void garbage_collect(void *ctx);

    intrusive_list_t<timer_t> timers;

public:
    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(resource_t resource, int events, linux_epoll_callback_t *cb);
    void forget_resource(resource_t resource, linux_epoll_callback_t *cb);
    
    timer_t *add_timer_internal(long ms, void (*callback)(void *ctx), void *ctx, bool once);
    void cancel_timer(timer_t *timer);
};


#endif // __EVENT_QUEUE_HPP__
