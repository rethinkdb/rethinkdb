
#include <sys/epoll.h>
#include <unistd.h>
#include "config.hpp"
#include "utils.hpp"
#include "event_queue.hpp"

void* aio_poll_handler(void *arg) {
    int res;
    io_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    event_queue_t *self = (event_queue_t*)arg;
    do {
        res = io_getevents(self->aio_context, 1, sizeof(events), events, NULL);
        check("Could not get AIO events", res < 0);
        for(int i = 0; i < res; i++) {
            if(self->event_handler) {
                self->event_handler(NULL);
            }
        }
    } while(1);
}

void* epoll_handler(void *arg) {
    int res;
    event_queue_t *self = (event_queue_t*)arg;
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    
    do {
        res = epoll_wait(self->epoll_fd, events, sizeof(events), -1);
        check("Waiting for an event failed", res == -1);

        for(int i = 0; i < res; i++) {
            if(events[i].events == EPOLLIN) {
                if(self->event_handler) {
                    self->event_handler(NULL);
                }
            }
            if(events[i].events == EPOLLRDHUP ||
               events[i].events == EPOLLERR ||
               events[i].events == EPOLLHUP) {
                queue_forget_resource(self, events[i].data.fd);
                close(events[i].data.fd);
            }
        }
    } while(1);
    return NULL;
}

void create_event_queue(event_queue_t *event_queue, int queue_id, event_handler_t event_handler) {
    int res;
    event_queue->queue_id = queue_id;
    
    // Create aio context
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &event_queue->aio_context);
    check("Could not setup aio context", res != 0);
    
    // Start aio poll thread
    res = pthread_create(&event_queue->aio_thread, NULL, aio_poll_handler, (void*)event_queue);
    check("Could not create aio_poll thread", res != 0);
    
    // Create a poll fd
    event_queue->epoll_fd = epoll_create(CONCURRENT_NETWORK_EVENTS_COUNT_HINT);
    check("Could not create epoll fd", event_queue->epoll_fd == -1);

    // Start the epoll thread
    res = pthread_create(&event_queue->epoll_thread, NULL, epoll_handler, (void*)event_queue);
    check("Could not create epoll thread", res != 0);
}

void destroy_event_queue(event_queue_t *event_queue) {
    close(event_queue->epoll_fd);
    io_destroy(event_queue->aio_context);
    // TODO: kill epoll and aio threads
}

void queue_watch_resource(event_queue_t *event_queue, resource_t resource) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    event.data.fd = resource;
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_ADD, resource, &event);
    check("Could not pass socket to worker", res != 0);
    // TODO: figure out edge vs. level triggering
}

void queue_forget_resource(event_queue_t *event_queue, resource_t resource) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    event.data.fd = resource;
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_DEL, resource, &event);
    check("Could remove socket from watching", res != 0);
}

