
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <pthread.h>
#include <libaio.h>
#include <queue>
#include "arch/resource.hpp"
#include "arch/io.hpp"
#include "event.hpp"
#include "corefwd.hpp"
#include "message_hub.hpp"
#include "config/cmd_args.hpp"
#include "buffer_cache/callbacks.hpp"
#include "perfmon.hpp"

typedef void (*event_handler_t)(event_queue_t*, event_t*);

// Inter-thread communication event (ITC)
enum itc_event_type_t {
    iet_shutdown = 1,
    iet_new_socket,
    iet_cache_synced,
};

struct itc_event_t {
    itc_event_type_t event_type;
    int data;
};

// Event queue structure
struct event_queue_t : public sync_callback<code_config_t> {
public:
    typedef code_config_t::cache_t cache_t;
    typedef code_config_t::conn_fsm_t conn_fsm_t;
    typedef code_config_t::fsm_list_t fsm_list_t;
    
public:
    event_queue_t(int queue_id, int _nqueues, event_handler_t event_handler,
                  worker_pool_t *parent_pool, cmd_config_t *cmd_config);
    void start_queue();
    
    // To safely shut down a group of event queues, call begin_stopping_queue() on each, then
    // finish_stopping_queue() on each. Then it is safe to delete them.
    void begin_stopping_queue();
    void finish_stopping_queue();
    ~event_queue_t();
    
    // Watching and forgetting resources (from the queue's POV)
    void watch_resource(resource_t resource, event_op_t event_op, void *state);
    void forget_resource(resource_t resource);

    // Posting an ITC message on the queue. The instance of
    // itc_event_t is serialized and need not be kept after the
    // function returns.
    void post_itc_message(itc_event_t *event);


    // Maintain a list of live FSMs
    void register_fsm(conn_fsm_t *fsm);
    void deregister_fsm(conn_fsm_t *fsm);

    void pull_messages_for_cpu(message_hub_t::msg_list_t *target);

    virtual void on_sync();

public:
    struct timer_t : public intrusive_list_node_t<timer_t>,
                     public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, timer_t> {
        
        friend class event_queue_t;
        
    private:
        // If 'false', the timer is repeating
        bool once;
        
        // If a repeating timer, this is the time between 'rings'
        long interval_ms;
        
        // This is the time (in ms since the server started) of the next 'ring'
        long next_time_in_ms;
        
        void (*callback)(void *ctx);
        void *context;
    };
    
    timer_t *add_timer(long ms, void (*callback)(void *ctx), void *ctx);
    timer_t *fire_timer_once(long ms, void (*callback)(void *ctx), void *ctx);
    void cancel_timer(timer_t *timer);

public:
    // TODO: be clear on what should and shouldn't be public here
    int queue_id;
    int nqueues;
    io_context_t aio_context;
    resource_t aio_notify_fd, core_notify_fd;
    resource_t timer_fd;
    long timer_ticks_since_server_startup;
    pthread_t epoll_thread;
    resource_t epoll_fd;
    resource_t itc_pipe[2];
    event_handler_t event_handler;
    // TODO: we should abstract live_fsms away from the queue. The
    // user of the queue provide an object that holds queue-local
    // state.
    fsm_list_t live_fsms;
    worker_pool_t *parent_pool;
    message_hub_t message_hub;

    // TODO: right now we only have one slice per event queue. We
    // should support multiple slices per queue.
    // TODO: implement slice writeback scheduling and admin tools.
    // Caches responsible for serving a particular queue
    cache_t *cache;

    io_calls_t iosys;

    // For performance monitoring
    perfmon_t perfmon;
    int total_connections, curr_connections;

#ifndef NDEBUG
    // Print debugging information designed to resolve a deadlock
    void deadlock_debug();
#endif
    
private:
    

    static void *epoll_handler(void *ctx);
    void process_timer_notify();
    static void garbage_collect(void *ctx);
    timer_t *add_timer_internal(long ms, void (*callback)(void *ctx), void *ctx, bool once);

    intrusive_list_t<timer_t> timers;
};

#endif // __EVENT_QUEUE_HPP__
