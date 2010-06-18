
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <pthread.h>
#include <libaio.h>
#include <queue>

#include "arch/resource.hpp"
#include "event.hpp"
#include "corefwd.hpp"
#include "message_hub.hpp"
#include "config/cmd_args.hpp"

typedef void (*event_handler_t)(event_queue_t*, event_t*);

// Inter-thread communication event (ITC)
enum itc_event_type_t {
    iet_shutdown,
    iet_new_socket
};

struct itc_event_t {
    itc_event_type_t event_type;
    int data;
};

// Event queue structure
struct event_queue_t {
public:
    typedef code_config_t::cache_t cache_t;
    typedef code_config_t::conn_fsm_t conn_fsm_t;
    typedef code_config_t::alloc_t alloc_t;
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

    void set_timer(timespec *, void (*callback)(void *ctx), void *ctx);

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
    // Caches responsible for serving a particular queue
    cache_t *cache;

private:
    struct timer {
        itimerspec it;
        void (*callback)(void *);
        void *context;
    };

    static void *epoll_handler(void *);
    void process_timer_notify();

    std::priority_queue<timer *> timers;
};

#endif // __EVENT_QUEUE_HPP__
