
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
    ~event_queue_t();
    void start_queue();
    
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

    void timer_add(timespec *, void (*callback)(void *ctx), void *ctx);
    void timer_once(timespec *, void (*callback)(void *ctx), void *ctx);

    virtual void on_sync();

public:
    // TODO: be clear on what should and shouldn't be public here
    int queue_id;
    int nqueues;
    io_context_t aio_context;
    resource_t aio_notify_fd, core_notify_fd;
    resource_t timer_fd;
    long total_expirations;
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

private:
    struct timer_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, timer_t> {
        itimerspec it;
        void (*callback)(void *ctx);
        void *context;
    };

    static void *epoll_handler(void *ctx);
    void process_timer_notify();
    static void garbage_collect(void *ctx);
    void timer_add_internal(timespec *, void (*callback)(void *ctx), void *ctx,
        bool once);

    struct timer_gt {
        bool operator()(const timer_t *t1, const timer_t *t2);
    };
    std::priority_queue<timer_t*, std::vector<timer_t*>, timer_gt> timers;
};

#endif // __EVENT_QUEUE_HPP__
