
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <pthread.h>
#include <libaio.h>
#include <queue>
#include <sys/epoll.h>
#include "arch/linux/io.hpp"
#include "event.hpp"
#include "corefwd.hpp"
#include "message_hub.hpp"
#include "config/cmd_args.hpp"

// Inter-thread communication event (ITC)
enum itc_event_type_t {
    iet_shutdown = 1,
    iet_new_socket,
    iet_finish_shutdown,
};

struct itc_event_t {
    itc_event_type_t event_type;
    void *data;
};

// Event queue structure
struct linux_event_queue_t {
public:
    linux_event_queue_t(int queue_id, int _nqueues,
                  worker_pool_t *parent_pool, worker_t *worker, cmd_config_t *cmd_config);
    void start_queue(worker_t *parent);
    
    // To safely shut down a group of event queues, call begin_stopping_queue() on each, then
    // finish_stopping_queue() on each. Then it is safe to delete them.
    void begin_stopping_queue();
    void finish_stopping_queue();
    ~linux_event_queue_t();

    // Posting an ITC message on the queue. The instance of
    // itc_event_t is serialized and need not be kept after the
    // function returns.
    void post_itc_message(itc_event_t *event);

    void pull_messages_for_cpu(message_hub_t::msg_list_t *target);

    void send_shutdown();
    
    // Can be called by any thread. Used by message_hub_t to inform the event_queue that it has
    // sent a message.
    void you_have_messages();
    
public:
    struct timer_t : public intrusive_list_node_t<timer_t>,
                     public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, timer_t> {
        
        friend class linux_event_queue_t;
    private:
        // If 'false', the timer is repeating
        bool once;
        
        // If a repeating timer, this is the time between 'rings'
        long interval_ms;
        
        // This is the time (in ms since the server started) of the next 'ring'
        long next_time_in_ms;
        
        // It's unsafe to remove arbitrary timers from the list as we iterate over
        // it, so instead we set the 'deleted' flag and then remove them in a
        // controlled fashion.
        bool deleted;
        
        void (*callback)(void *ctx);
        void *context;
    };
    
    timer_t *add_timer(long ms, void (*callback)(void *ctx), void *ctx);
    timer_t *fire_timer_once(long ms, void (*callback)(void *ctx), void *ctx);
    void cancel_timer(timer_t *timer);

public:
    // TODO: be clear on what should and shouldn't be public here
    io_context_t aio_context;
    resource_t aio_notify_fd, core_notify_fd;
    resource_t timer_fd;
    long timer_ticks_since_server_startup;
    pthread_t epoll_thread;
    fd_t epoll_fd;
    fd_t itc_pipe[2];
    worker_pool_t *parent_pool;
    worker_t *parent;
    message_hub_t message_hub;
    linux_io_calls_t iosys;
    // We store this as a class member because forget_resource needs
    // to go through the events and remove queued messages for
    // resources that are being destroyed.
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    int nevents;

private:
    static void *epoll_handler(void *ctx);
    void process_timer_notify();
    static void garbage_collect(void *ctx);
    timer_t *add_timer_internal(long ms, void (*callback)(void *ctx), void *ctx, bool once);

    intrusive_list_t<timer_t> timers;

public:
    // These should only be called by the event queue itself or by the linux_* classes
    void watch_resource(resource_t resource, event_op_t event_op, void *state);
    void forget_resource(resource_t resource, void *state);
};


#endif // __EVENT_QUEUE_HPP__
