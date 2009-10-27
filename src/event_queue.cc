
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <strings.h>
#include "config.hpp"
#include "utils.hpp"
#include "event_queue.hpp"

// TODO: report event queue statistics.

void process_aio_notify(event_queue_t *self) {
    int res;
    eventfd_t nevents;
    res = eventfd_read(self->aio_notify_fd, &nevents);
    check("Could not read aio_notify_fd value", res != 0);

    // TODO: we need an array allocator here
    io_event *events = (io_event*)malloc(&self->allocator, sizeof(io_event) * nevents);
    
    // Grab the events
    nevents = io_getevents(self->aio_context, 1, 1, events, NULL);
    check("Waiting for AIO event failed", nevents != 1);
        
    // Process the events
    for(int i = 0; i < nevents; i++) {
        if(self->event_handler) {
            event_t qevent;
            bzero((char*)&qevent, sizeof(qevent));
            qevent.event_type = et_disk_event;
            iocb *op = (iocb*)events[i].obj;
            qevent.source = op->aio_fildes;
            qevent.result = events[i].res;
            qevent.buf = op->u.c.buf;
            qevent.state = events[i].data;
            if(op->aio_lio_opcode == IO_CMD_PREAD)
                qevent.op = eo_read;
            else
                qevent.op = eo_write;
            self->event_handler(self, &qevent);
        }
        free(&self->allocator, events[i].obj);
    }

    free(&self->allocator, events);
}

void* epoll_handler(void *arg) {
    int res;
    event_queue_t *self = (event_queue_t*)arg;
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    
    do {
        res = epoll_wait(self->epoll_fd, events, sizeof(events), -1);
        // epoll_wait might return with EINTR in some cases (in
        // particular under GDB), we just need to retry.
        if(res == -1 && errno == EINTR) {
            if(self->dying)
                break;
            else
                continue;
        }
        check("Waiting for epoll events failed", res == -1);

        for(int i = 0; i < res; i++) {
            if(events[i].data.fd == self->aio_notify_fd) {
                process_aio_notify(self);
                continue;
            }
            if(events[i].events == EPOLLIN ||
               events[i].events == EPOLLOUT)
            {
                if(self->event_handler) {
                    event_t qevent;
                    bzero((char*)&qevent, sizeof(qevent));
                    qevent.event_type = et_sock_event;
                    qevent.source = events[i].data.fd;
                    qevent.state = events[i].data.ptr;
                    if(events[i].events & EPOLLIN)
                        qevent.op = eo_read;
                    else
                        qevent.op = eo_write;
                    self->event_handler(self, &qevent);
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

void create_event_queue(event_queue_t *event_queue, int queue_id, event_handler_t event_handler,
                        worker_pool_t *parent_pool) {
    int res;
    event_queue->queue_id = queue_id;
    event_queue->event_handler = event_handler;
    event_queue->parent_pool = parent_pool;
    event_queue->dying = false;

    // Initialize the allocator
    create_allocator(&event_queue->allocator, ALLOCATOR_WORKER_HEAP);
    
    // Create aio context
    event_queue->aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &event_queue->aio_context);
    check("Could not setup aio context", res != 0);
    
    // Create a poll fd
    event_queue->epoll_fd = epoll_create(CONCURRENT_NETWORK_EVENTS_COUNT_HINT);
    check("Could not create epoll fd", event_queue->epoll_fd == -1);

    // Start the epoll thread
    res = pthread_create(&event_queue->epoll_thread, NULL, epoll_handler, (void*)event_queue);
    check("Could not create epoll thread", res != 0);

    // Create aio notify fd
    event_queue->aio_notify_fd = eventfd(0, 0);
    check("Could not create aio notification fd", event_queue->aio_notify_fd == -1);

    res = fcntl(event_queue->aio_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make aio notify socket non-blocking", res != 0);

    queue_watch_resource(event_queue, event_queue->aio_notify_fd, eo_read, NULL);
    
    // Set thread affinity
    int ncpus = get_cpu_count();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(queue_id % ncpus, &mask);
    res = pthread_setaffinity_np(event_queue->epoll_thread, sizeof(cpu_set_t), &mask);
    check("Could not set thread affinity", res != 0);
}

void destroy_event_queue(event_queue_t *event_queue) {
    int res;

    event_queue->dying = true;

    // Kill the threads
    res = pthread_kill(event_queue->epoll_thread, SIGTERM);
    check("Could not send kill signal to epoll thread", res != 0);

    // Wait for the threads to die
    res = pthread_join(event_queue->epoll_thread, NULL);
    check("Could not join with epoll thread", res != 0);
    
    // Cleanup resources
    close(event_queue->aio_notify_fd);
    close(event_queue->epoll_fd);
    io_destroy(event_queue->aio_context);
    destroy_allocator(&event_queue->allocator);
}

void queue_watch_resource(event_queue_t *event_queue, resource_t resource,
                          event_op_t watch_mode, void *state) {
    epoll_event event;
    
    // only trigger if new events come in
    event.events = EPOLLET;
    if(watch_mode == eo_read)
        event.events |= EPOLLIN;
    else
        event.events |= EPOLLOUT;
    event.data.ptr = state;
    
    event.data.fd = resource;
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_ADD, resource, &event);
    check("Could not pass socket to worker", res != 0);
}

void queue_forget_resource(event_queue_t *event_queue, resource_t resource) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    event.data.fd = resource;
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_DEL, resource, &event);
    check("Could remove socket from watching", res != 0);
}

