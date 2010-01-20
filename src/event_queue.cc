
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <new>
#include <algorithm>
#include "config.hpp"
#include "utils.hpp"
#include "event_queue.hpp"

// TODO: report event queue statistics.

// Some forward declarations
void queue_init_timer(event_queue_t *event_queue, time_t secs);
void queue_stop_timer(event_queue_t *event_queue);

void process_aio_notify(event_queue_t *self) {
    int res, nevents;
    eventfd_t nevents_total;
    res = eventfd_read(self->aio_notify_fd, &nevents_total);
    check("Could not read aio_notify_fd value", res != 0);

    // Note: O(1) array allocators are hard. To avoid all the
    // complexity, we'll use a fixed sized array and call io_getevents
    // multiple times if we have to (which should be very unlikely,
    // anyway).
    io_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    
    do {
        // Grab the events. Note: we need to make sure we don't read
        // more than nevents_total, otherwise we risk reading an io
        // event and getting an eventfd for this read event later due
        // to the way the kernel is structured. Better avoid this
        // complexity (hence std::min below).
        nevents = io_getevents(self->aio_context, 0,
                               std::min((int)nevents_total, MAX_IO_EVENT_PROCESSING_BATCH_SIZE),
                               events, NULL);
        check("Waiting for AIO event failed", nevents < 1);
        
        // Process the events
        for(int i = 0; i < nevents; i++) {
            if(self->event_handler) {
                event_t qevent;
                bzero((char*)&qevent, sizeof(qevent));
                qevent.event_type = et_disk;
                iocb *op = (iocb*)events[i].obj;
                qevent.result = events[i].res;
                qevent.buf = op->u.c.buf;
                qevent.state = (event_state_t*)events[i].data;
                if(op->aio_lio_opcode == IO_CMD_PREAD)
                    qevent.op = eo_read;
                else
                    qevent.op = eo_write;
                self->event_handler(self, &qevent);
            }
            self->alloc.free(events[i].obj);
        }
        nevents_total -= nevents;
    } while(nevents_total > 0);
}

void process_timer_notify(event_queue_t *self) {
    int res;
    eventfd_t nexpirations;
    res = eventfd_read(self->timer_fd, &nexpirations);
    check("Could not read timer_fd value", res != 0);

    self->total_expirations += nexpirations;

    // Internal ops to perform on the timer
    if(self->total_expirations % ALLOC_GC_IN_TICKS == 0) {
        // Perform allocator gc
        self->alloc.gc();
    }

    // Let queue user handle the event, if they wish
    if(self->event_handler) {
        event_t qevent;
        bzero((char*)&qevent, sizeof(qevent));
        qevent.event_type = et_timer;
        qevent.state = (event_state_t*)&self->timer_fd;
        qevent.result = nexpirations;
        qevent.op = eo_read;
        self->event_handler(self, &qevent);
    }
}

int process_itc_notify(event_queue_t *self) {
    int res;
    itc_event_t event;

    // Read the event
    res = read(self->itc_pipe[0], &event, sizeof(event));
    check("Could not read itc event", res != sizeof(event));

    // Process the event
    switch(event.event_type) {
    case iet_shutdown:
        return 1;
        break;
    case iet_new_socket:
        // The state will be freed within the fsm when the socket is
        // closed (or killed for a variety of possible reasons)
        fsm_state_t *state = self->alloc.malloc<fsm_state_t>();
        fsm_init_state(state);
        state->source = event.data;
        state->prev = NULL;
        state->next = NULL;

        self->live_fsms.push_back(state);

        queue_watch_resource(self, event.data, eo_rdwr, state);
        printf("Opened socket %d\n", event.data);
        break;
    }

    return 0;
}

int process_network_notify(event_queue_t *self, epoll_event *event) {
    if(event->events & EPOLLIN ||
       event->events & EPOLLOUT)
    {
        if(self->event_handler) {
            event_t qevent;
            bzero((char*)&qevent, sizeof(qevent));
            qevent.event_type = et_sock;
            qevent.state = (event_state_t*)(event->data.ptr);
            if((event->events & EPOLLIN) && !(event->events & EPOLLOUT)) {
                qevent.op = eo_read;
            }
            else if(!(event->events & EPOLLIN) && (event->events & EPOLLOUT)) {
                qevent.op = eo_write;
            } else {
                qevent.op = eo_rdwr;
            }
            self->event_handler(self, &qevent);
        }
    } else if(event->events == EPOLLRDHUP ||
              event->events == EPOLLERR ||
              event->events == EPOLLHUP)
    {
        fsm_destroy_state((fsm_state_t*)(event->data.ptr), self);
    } else {
        check("epoll_wait came back with an unhandled event", 1);
    }
}

void* epoll_handler(void *arg) {
    int res;
    event_queue_t *self = (event_queue_t*)arg;
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    
    do {
        res = epoll_wait(self->epoll_fd, events, MAX_IO_EVENT_PROCESSING_BATCH_SIZE, -1);
        // epoll_wait might return with EINTR in some cases (in
        // particular under GDB), we just need to retry.
        if(res == -1 && errno == EINTR) {
            continue;
        }
        check("Waiting for epoll events failed", res == -1);

        // TODO: instead of processing the events immediately, we
        // might want to queue them up and then process the queue in
        // bursts. This might reduce response time but increase
        // overall throughput because it will minimize cache faults
        // associated with instructions as well as data structures
        // (see section 7 [CPU scheduling] in B-tree Indexes and CPU
        // Caches by Goetz Graege and Pre-Ake Larson).
        for(int i = 0; i < res; i++) {
            resource_t source = ((event_state_t*)events[i].data.ptr)->source;
            if(source == self->aio_notify_fd) {
                process_aio_notify(self);
            } else if(source == self->timer_fd) {
                process_timer_notify(self);
            } else if(source == self->itc_pipe[0]) {
                if(process_itc_notify(self)) {
                    // We're shutting down
                    return NULL;
                }
            } else {
                process_network_notify(self, &events[i]);
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
    event_queue->timer_fd = -1;
    event_queue->total_expirations = 0;

    // Initialize the allocator using placement new
    new ((void*)&event_queue->alloc) event_queue_t::small_obj_alloc_t();

    // Initialize the list of live fsms using placement new
    new ((void*)&event_queue->live_fsms) fsm_list_t();
    
    // Create aio context
    event_queue->aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &event_queue->aio_context);
    check("Could not setup aio context", res != 0);
    
    // Create a poll fd
    event_queue->epoll_fd = epoll_create1(0);
    check("Could not create epoll fd", event_queue->epoll_fd == -1);

    // Start the epoll thread
    res = pthread_create(&event_queue->epoll_thread, NULL, epoll_handler, (void*)event_queue);
    check("Could not create epoll thread", res != 0);

    // Create aio notify fd
    event_queue->aio_notify_fd = eventfd(0, 0);
    check("Could not create aio notification fd", event_queue->aio_notify_fd == -1);

    res = fcntl(event_queue->aio_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make aio notify fd non-blocking", res != 0);

    queue_watch_resource(event_queue, event_queue->aio_notify_fd, eo_read, (event_state_t*)&(event_queue->aio_notify_fd));
    
    // Create ITC notify pipe
    res = pipe(event_queue->itc_pipe);
    check("Could not create itc pipe", res != 0);

    res = fcntl(event_queue->itc_pipe[0], F_SETFL, O_NONBLOCK);
    check("Could not make itc pipe non-blocking (read end)", res != 0);

    res = fcntl(event_queue->itc_pipe[1], F_SETFL, O_NONBLOCK);
    check("Could not make itc pipe non-blocking (write end)", res != 0);

    queue_watch_resource(event_queue, event_queue->itc_pipe[0], eo_read, (event_state_t*)&(event_queue->itc_pipe[0]));
    
    // Set thread affinity
    int ncpus = get_cpu_count();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(queue_id % ncpus, &mask);
    res = pthread_setaffinity_np(event_queue->epoll_thread, sizeof(cpu_set_t), &mask);
    check("Could not set thread affinity", res != 0);

    // Start the timer
    queue_init_timer(event_queue, TIMER_TICKS_IN_SECS);
}

// This method gets called from the main (worker pool's thread),
// unlike others that should only be called from the event_queue
// thread.
void destroy_event_queue(event_queue_t *event_queue) {
    int res;

    // Stop the timer
    queue_stop_timer(event_queue);

    // Kill the poll thread
    itc_event_t event;
    event.event_type = iet_shutdown;
    post_itc_message(event_queue, &event);

    // Wait for the poll thread to die
    res = pthread_join(event_queue->epoll_thread, NULL);
    check("Could not join with epoll thread", res != 0);
    
    // Cleanup remaining fsms
    fsm_state_t *state = event_queue->live_fsms.head();
    while(state) {
        fsm_destroy_state(state, event_queue);
        state = event_queue->live_fsms.head();
    }

    // Cleanup resources
    res = close(event_queue->epoll_fd);
    check("Could not close epoll_fd", res != 0);
    
    res = close(event_queue->itc_pipe[0]);
    check("Could not close itc pipe (read end)", res != 0);

    res = close(event_queue->itc_pipe[1]);
    check("Could not close itc pipe (write end)", res != 0);
    
    res = close(event_queue->aio_notify_fd);
    check("Could not close aio_notify_fd", res != 0);
    
    res = io_destroy(event_queue->aio_context);
    check("Could not destroy aio_context", res != 0);
    
    (&event_queue->live_fsms)->~fsm_list_t();
    (&event_queue->alloc)->~object_static_alloc_t();
}

void queue_watch_resource(event_queue_t *event_queue, resource_t resource,
                          event_op_t watch_mode, event_state_t *state) {
    assert(state);
    epoll_event event;
    
    // only trigger if new events come in
    event.events = EPOLLET;
    if(watch_mode == eo_read) {
        event.events |= EPOLLIN;
    } else if(watch_mode == eo_write) {
        event.events |= EPOLLOUT;
    } else if(watch_mode == eo_rdwr) {
        event.events |= EPOLLIN;
        event.events |= EPOLLOUT;
    } else {
        check("Invalid watch mode", 1);
    }

    event.data.ptr = (void*)state;
    
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_ADD, resource, &event);
    check("Could not pass socket to worker", res != 0);
}

void queue_forget_resource(event_queue_t *event_queue, resource_t resource) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    int res = epoll_ctl(event_queue->epoll_fd, EPOLL_CTL_DEL, resource, &event);
    check("Couldn't remove socket from watching", res != 0);
}

void queue_init_timer(event_queue_t *event_queue, time_t secs) {
    int res = -1;

    // Kill the old timer first (if necessary)
    if(event_queue->timer_fd != -1) {
        queue_stop_timer(event_queue);
    }

    // Create the timer
    event_queue->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    check("Could not create timer", event_queue->timer_fd < 0);

    // Set the timer_fd to be nonblocking
    res = fcntl(event_queue->timer_fd, F_SETFL, O_NONBLOCK);
    check("Could not make timer non-blocking", res != 0);

    // Arm the timer
    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    
    timer_spec.it_value.tv_sec = secs;
    timer_spec.it_value.tv_nsec = 0;
    timer_spec.it_interval.tv_sec = secs;
    timer_spec.it_interval.tv_nsec = 0;
    
    res = timerfd_settime(event_queue->timer_fd, 0, &timer_spec, NULL);
    check("Could not arm the timer.", res != 0);

    // Watch the timer
    queue_watch_resource(event_queue, event_queue->timer_fd, eo_read, (event_state_t*)&(event_queue->timer_fd));
}

void queue_stop_timer(event_queue_t *event_queue) {
    if(event_queue->timer_fd == -1)
        return;
    
    // Stop watching the timer
    queue_forget_resource(event_queue, event_queue->timer_fd);
    
    int res = -1;
    // Disarm the timer (should happen automatically on close, but what the hell)
    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    res = timerfd_settime(event_queue->timer_fd, 0, &timer_spec, NULL);
    check("Could not disarm the timer.", res != 0);
    
    // Close the timer fd
    res = close(event_queue->timer_fd);
    check("Could not close the timer.", res != 0);
    event_queue->timer_fd = -1;
}

void post_itc_message(event_queue_t *event_queue, itc_event_t *event) {
    int res;
    // Note: This will be atomic as long as sizeof(itc_event_t) <
    // PIPE_BUF
    res = write(event_queue->itc_pipe[1], event, sizeof(itc_event_t));
    check("Could not post message to itc queue", res != sizeof(itc_event_t));
    
    // TODO: serialization of the whole structure isn't as fast as
    // serializing a pointer (but that would involve heap allocation,
    // and a thread-safe allocator, which is a whole other can of
    // worms). We should revisit this when it's more clear how ITC is
    // used.
}

