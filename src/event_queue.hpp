
#ifndef __EVENT_QUEUE_HPP__
#define __EVENT_QUEUE_HPP__

#include "event_queue_impl.hpp"

// Event handling
struct event_t {
    resource_t resource;
};

typedef void (*event_handler_t)(event_queue_t*, event_t*);

// Event queue initialization/destruction
void create_event_queue(event_queue_t *event_queue, int queue_id, event_handler_t event_handler);
void destroy_event_queue(event_queue_t *event_queue);

// Event queue operation
void queue_watch_resource(event_queue_t *event_queue, resource_t resource);
void queue_forget_resource(event_queue_t *event_queue, resource_t resource);

#endif // __EVENT_QUEUE_HPP__

