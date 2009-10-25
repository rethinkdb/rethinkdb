
#ifndef __EVENT_QUEUE_IMPL_HPP__
#define __EVENT_QUEUE_IMPL_HPP__

#include <pthread.h>
#include <libaio.h>

// Forward declarations
struct event_t;

// Resource type. In linux it's an int for file descriptors
typedef int resource_t;

// Event queue structure
struct event_queue_t {
    int queue_id;
    
    pthread_t aio_thread;
    io_context_t aio_context;

    pthread_t epoll_thread;
    int epoll_fd;

    void (*event_handler)(event_queue_t*, event_t*);
};

#endif // __EVENT_QUEUE_IMPL_HPP__

