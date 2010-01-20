
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include <pthread.h>
#include <libaio.h>
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/object_static.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "arch/common.hpp"
#include "fsm.hpp"
#include "common.hpp"
#include "config.hpp"

// Queue event handling
enum event_type_t {
    et_disk, et_sock, et_timer
};
enum event_op_t {
    eo_read, eo_write, eo_rdwr
};
struct event_t {
    event_type_t event_type;
    event_op_t op;

    // State associated with the communication (must have been passed
    // to watch_resource).
    event_state_t *state;

    /* For event_type == et_disk_event, contains the result of the IO
     * operation. For event_type == et_timer_event, contains the
     * number of experiations of the timer that have occurred. */
    int result;

    /* For event_type == et_disk_event */
    void *buf;    // Location of the buffer where data was copied (for read events)
};
struct event_queue_t;
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
struct worker_pool_t;
typedef buffer_t<IO_BUFFER_SIZE> io_buffer_t;
struct event_queue_t {
public:
    event_queue_t(int queue_id, event_handler_t event_handler,
                  worker_pool_t *parent_pool);
    ~event_queue_t();
    
    // Watching and forgetting resources (from the queue's POV)
    void watch_resource(resource_t resource, event_op_t event_op,
                        event_state_t *state);
    void forget_resource(resource_t resource);

    // Posting an ITC message on the queue. The instance of
    // itc_event_t is serialized and need not be kept after the
    // function returns.
    void post_itc_message(itc_event_t *event);

public:
    // TODO: be clear on what should and shouldn't be public here
    int queue_id;
    io_context_t aio_context;
    resource_t aio_notify_fd;
    resource_t timer_fd;
    long total_expirations;
    pthread_t epoll_thread;
    resource_t epoll_fd;
    resource_t itc_pipe[2];
    event_handler_t event_handler;
    // TODO: add a checking allocator (check if malloc returns NULL)
    // TODO: we should abstract the allocator away from here via a
    // template argument. This would probably require changing the
    // queue to be more C++ friendly.
    typedef object_static_alloc_t<
        dynamic_pool_alloc_t<alloc_stats_t<pool_alloc_t<memalign_alloc_t<> > > >,
        iocb, fsm_state_t, io_buffer_t> small_obj_alloc_t;
    small_obj_alloc_t alloc;
    // TODO: we should abstract live_fsms away from the queue. The
    // user of the queue provide an object that holds queue-local
    // state.
    fsm_list_t live_fsms;
    worker_pool_t *parent_pool;
};

#endif // __EVENT_QUEUE_HPP__

