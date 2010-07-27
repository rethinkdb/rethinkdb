
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
#include <string>
#include <sstream>
#include "config/args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include "alloc/memalign.hpp"
#include "alloc/pool.hpp"
#include "alloc/dynamic_pool.hpp"
#include "alloc/stats.hpp"
#include "alloc/alloc_mixin.hpp"
#include "worker_pool.hpp"
#include "arch/io.hpp"
#include "serializer/in_place.hpp"
#include "buffer_cache/mirrored.hpp"
#include "buffer_cache/page_map/array.hpp"
#include "buffer_cache/page_repl/page_repl_random.hpp"
#include "buffer_cache/writeback/writeback.hpp"
#include "buffer_cache/concurrency/rwi_conc.hpp"
#include "btree/get_fsm.hpp"
#include "btree/set_fsm.hpp"
#include "btree/incr_decr_fsm.hpp"
#include "btree/delete_fsm.hpp"
#include "conn_fsm.hpp"
#include "request.hpp"
#include "event_queue.hpp"
#include "cpu_context.hpp"

static const int NSEC_IN_SEC = 1000 * 1000 * 1000;

// TODO: report event queue statistics.

// TODO: at this point the event queue contains all kinds of code that
// really doesn't belong here. When we ship v1, we should sit down,
// think what should belong to the event queue and what shouldn't, and
// abstract all the pieces that really should be somewhere else.

// Some forward declarations
void queue_init_timer(event_queue_t *event_queue, uint64_t ms);
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
            self->iosys.aio_notify((iocb*)events[i].obj, events[i].res);
        }
        nevents_total -= nevents;
    } while(nevents_total > 0);
}

void event_queue_t::process_timer_notify() {
    int res;
    eventfd_t nexpirations;

    res = eventfd_read(timer_fd, &nexpirations);
    check("Could not read timer_fd value", res != 0);
    
    timer_ticks_since_server_startup += nexpirations;
    long time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS;

    /* Execute any expired timers. */
    intrusive_list_t<timer_t>::iterator t;
    for (t = timers.begin(); t != timers.end(); ) {
        timer_t *timer = &*t;
        t++; // Increment now because the list may be mutated
        
        if (time_in_ms > timer->next_time_in_ms) {
            
            // Note that a repeating timer may have "expired" multiple times since the last time
            // process_timer_notify() was called. However, everything that uses the timer mechanism
            // right now works better if the timer's callback only happens once. Perhaps there
            // should be a flag on the timer that determines whether to call the timer's callback
            // more than once or not.
            
            timer->callback(timer->context);
            
            if (timer->once) {
                timers.remove(timer);
                delete timer;
            } else {
                timer->next_time_in_ms = time_in_ms + timer->interval_ms;
            }
        }
    }
}

int process_itc_notify(event_queue_t *self) {
    int res;
    itc_event_t event;

    // Read the event
    while(1) {
        res = read(self->itc_pipe[0], &event, sizeof(event));
        if(res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;
        check("Could not read itc event", res != sizeof(event));

        // Process the event
        switch(event.event_type) {
        case iet_shutdown:
        case iet_cache_synced:
            return event.event_type;
            break;
        case iet_new_socket:
            // The state will be freed within the fsm when the socket is
            // closed (or killed for a variety of possible reasons)
            event_queue_t::conn_fsm_t *state =
                new event_queue_t::conn_fsm_t(event.data, self);
            self->register_fsm(state);
            self->total_connections++;
            self->curr_connections++;
            break;
        }
    }

    return 0;
}

void process_network_notify(event_queue_t *self, epoll_event *event) {
    if(event->events & EPOLLIN || event->events & EPOLLOUT) {
        if(self->event_handler) {
            event_t qevent;
            bzero((char*)&qevent, sizeof(qevent));
            qevent.event_type = et_sock;
            qevent.state = event->data.ptr;
            if((event->events & EPOLLIN) && !(event->events & EPOLLOUT)) {
                qevent.op = eo_read;
            } else if(!(event->events & EPOLLIN) && (event->events & EPOLLOUT)) {
                qevent.op = eo_write;
            } else {
                qevent.op = eo_rdwr;
            }
            self->event_handler(self, &qevent);
        }
    } else if(event->events == EPOLLRDHUP ||
              event->events == EPOLLERR ||
              event->events == EPOLLHUP) {
        self->deregister_fsm((event_queue_t::conn_fsm_t*)(event->data.ptr));
    } else {
        check("epoll_wait came back with an unhandled event", 1);
    }
}

void process_cpu_core_notify(event_queue_t *self, message_hub_t::msg_list_t *messages) {
    
    message_hub_t::msg_list_t::iterator m;
    for (m = messages->begin(); m != messages->end(); ) {
        cpu_message_t &msg = *m;
        m ++;
        
        /* Remove the message from our list so that the event handler can put it into another list
        without violating intrusive_list_t's internal corruption checks. Otherwise this would be
        unnecessary because our list is a temporary that will be discarded as soon as the function
        returns. */
        messages->remove(&msg);
        
        // Pass the event to the handler
        event_t cpu_event;
        cpu_event.event_type = et_cpu_event;
        cpu_event.state = &msg;
        self->event_handler(self, &cpu_event);
    }
    
    assert(messages->empty());

    // Note, the event handler is responsible for the deallocation
    // of cpu messages. For btree_fsms, for example this currently
    // occurs in build_response.
}

void *event_queue_t::epoll_handler(void *arg) {
    int res;
    event_queue_t *self = (event_queue_t*)arg;
    epoll_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];
    bool shutting_down = false; // True btw. iet_shutdown and iet_cache_synced.
    typedef std::vector<event_queue_t::conn_fsm_t*, gnew_alloc<conn_fsm_t*> > shutdown_fsms_t;
    shutdown_fsms_t shutdown_fsms;

    // First, set the cpu context structure
    get_cpu_context()->event_queue = self;
    
    // Cannot call this until we are in the correct thread and have an event queue
    self->cache->start();

    // Now, initialize the hardcoded (garbage collection)
    self->add_timer(ALLOC_GC_INTERVAL_MS, self->garbage_collect, NULL);
    
    // Now, start the loop
    do {
        // Grab the events from the kernel!
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
            resource_t source = reinterpret_cast<intptr_t>(events[i].data.ptr);
            if(source == self->aio_notify_fd) {
                process_aio_notify(self);
            } else if(source == self->timer_fd) {
                self->process_timer_notify();
            } else if(source == self->core_notify_fd) {
                // Great, we needed to wake up to process batch jobs
                // from other cores, no need to do anything here.
            } else if(source == self->itc_pipe[0]) {
                switch (process_itc_notify(self)) {
                case iet_shutdown: // We've been asked to shut down
                    assert(!shutting_down);
                    shutting_down = true;
                    // Clean up some objects that should be deleted on the same
                    // core as where they were allocated.

                    // Clean up remaining fsms
                    for (event_queue_t::conn_fsm_t *state =
                         self->live_fsms.head(); state != NULL;
                         state = self->live_fsms.head()) {
                        self->deregister_fsm(state);
                        shutdown_fsms.push_back(state);
                    }

                    self->cache->shutdown(self); // Initiate final cache flush
                    break;
                case iet_cache_synced:
                    assert(shutting_down);
                    goto breakout;
                }
            } else {
                process_network_notify(self, &events[i]);
            }
        }

        // We're done with the current batch of events, process cross
        // CPU requests
        if (!shutting_down) {
            message_hub_t::msg_list_t cpu_requests;
            self->pull_messages_for_cpu(&cpu_requests);
            process_cpu_core_notify(self, &cpu_requests);

            // Push the messages we collected in this batch for other CPUs
            self->message_hub.push_messages();
        }
    } while(1);

breakout:
    // Stop the timers
    queue_stop_timer(self);

    // Delete the registered timers
    while(!self->timers.empty()) {
        timer_t *t = self->timers.head();
        self->timers.remove(t);
        delete t;
    }

    for (shutdown_fsms_t::iterator it = shutdown_fsms.begin(); it != shutdown_fsms.end(); ++it)
        delete *it;
    gdelete(self->cache);

    return tls_small_obj_alloc_accessor<alloc_t>::allocs_tl;
}

event_queue_t::event_queue_t(int queue_id, int _nqueues, event_handler_t event_handler,
                             worker_pool_t *parent_pool, cmd_config_t *cmd_config)
    : iosys(this), total_connections(0), curr_connections(0)
{
    int res;
    this->queue_id = queue_id;
    this->nqueues = _nqueues;
    this->event_handler = event_handler;
    this->parent_pool = parent_pool;
    this->timer_fd = -1;
    this->timer_ticks_since_server_startup = 0;

    // Register some perfmon variables
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "total_connections", (void*)&total_connections));
    perfmon.monitor(var_monitor_t(var_monitor_t::vt_int, "curr_connections", (void*)&curr_connections));

    // Create aio context
    this->aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &this->aio_context);
    check("Could not setup aio context", res != 0);
    
    // Create a poll fd
    this->epoll_fd = epoll_create1(0);
    check("Could not create epoll fd", this->epoll_fd == -1);

    // Create aio notify fd
    this->aio_notify_fd = eventfd(0, 0);
    check("Could not create aio notification fd", this->aio_notify_fd == -1);

    res = fcntl(this->aio_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make aio notify fd non-blocking", res != 0);

    watch_resource(this->aio_notify_fd, eo_read, (void*)this->aio_notify_fd);

    // Create notify fd for other cores that send work to us
    this->core_notify_fd = eventfd(0, 0);
    check("Could not create core notification fd", this->core_notify_fd == -1);

    res = fcntl(this->core_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make core notify fd non-blocking", res != 0);

    watch_resource(this->core_notify_fd, eo_read, (void*)this->core_notify_fd);
    
    // Create ITC notify pipe
    res = pipe(this->itc_pipe);
    check("Could not create itc pipe", res != 0);

    res = fcntl(this->itc_pipe[0], F_SETFL, O_NONBLOCK);
    check("Could not make itc pipe non-blocking (read end)", res != 0);

    res = fcntl(this->itc_pipe[1], F_SETFL, O_NONBLOCK);
    check("Could not make itc pipe non-blocking (write end)", res != 0);

    watch_resource(this->itc_pipe[0], eo_read, (void*)this->itc_pipe[0]);
    
    // Init the cache
    typedef std::basic_string<char, std::char_traits<char>, gnew_alloc<char> > rdbstring_t;
    typedef std::basic_stringstream<char, std::char_traits<char>, gnew_alloc<char> > rdbstringstream_t;
    rdbstringstream_t out;
    rdbstring_t str((char*)cmd_config->db_file_name);
    out << queue_id;
    str += out.str();
    cache = gnew<cache_t>(
        (char*)str.c_str(),
        BTREE_BLOCK_SIZE,
        cmd_config->max_cache_size / nqueues,
        cmd_config->wait_for_flush,
        cmd_config->flush_timer_ms,
        cmd_config->flush_threshold_percent);
}

void event_queue_t::start_queue() {
    // Start the epoll thread
    int res = pthread_create(&this->epoll_thread, NULL, epoll_handler, (void*)this);
    check("Could not create epoll thread", res != 0);

    // Set thread affinity
    int ncpus = get_cpu_count();
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(queue_id % ncpus, &mask);
    res = pthread_setaffinity_np(this->epoll_thread, sizeof(cpu_set_t), &mask);
    check("Could not set thread affinity", res != 0);

    // Start the timer
    queue_init_timer(this, TIMER_TICKS_IN_MS);
}

void event_queue_t::begin_stopping_queue() {
    // Kill the poll thread
    itc_event_t event;
    bzero(&event, sizeof event);
    event.event_type = iet_shutdown;
    post_itc_message(&event);
}

void event_queue_t::finish_stopping_queue() {
    int res;
    
    // Wait for the poll thread to die
    void *allocs_tl = NULL;
    res = pthread_join(this->epoll_thread, &allocs_tl);
    check("Could not join with epoll thread", res != 0);
    parent_pool->all_allocs.push_back(allocs_tl);
}

event_queue_t::~event_queue_t()
{
    int res;
    
    res = close(this->epoll_fd);
    check("Could not close epoll_fd", res != 0);
    
    res = close(this->itc_pipe[0]);
    check("Could not close itc pipe (read end)", res != 0);

    res = close(this->itc_pipe[1]);
    check("Could not close itc pipe (write end)", res != 0);
    
    res = close(this->core_notify_fd);
    check("Could not close core_notify_fd", res != 0);
    
    res = close(this->aio_notify_fd);
    check("Could not close aio_notify_fd", res != 0);
    
    res = io_destroy(this->aio_context);
    check("Could not destroy aio_context", res != 0);
}

void event_queue_t::watch_resource(resource_t resource, event_op_t watch_mode,
                                   void *state) {
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

    event.data.ptr = state;
    
    int res = epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, resource, &event);
    check("Could not pass socket to worker", res != 0);
}

void event_queue_t::forget_resource(resource_t resource) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    int res = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, resource, &event);
    check("Couldn't remove socket from watching", res != 0);
}

void queue_init_timer(event_queue_t *event_queue, uint64_t ms) {
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
    
    if(ms >= 1000) {
        timer_spec.it_value.tv_sec = ms / 1000;
        timer_spec.it_value.tv_nsec = 0;
        timer_spec.it_interval.tv_sec = ms / 1000;
        timer_spec.it_interval.tv_nsec = 0;
    } else {
        timer_spec.it_value.tv_sec = 0;
        timer_spec.it_value.tv_nsec = ms * 1000000L;
        timer_spec.it_interval.tv_sec = 0;
        timer_spec.it_interval.tv_nsec = ms * 1000000L;
    }
    
    res = timerfd_settime(event_queue->timer_fd, 0, &timer_spec, NULL);
    check("Could not arm the timer.", res != 0);

    // Watch the timer
    event_queue->watch_resource(event_queue->timer_fd, eo_read, (void*)event_queue->timer_fd);
}

void queue_stop_timer(event_queue_t *event_queue) {
    if(event_queue->timer_fd == -1)
        return;
    
    // Stop watching the timer
    event_queue->forget_resource(event_queue->timer_fd);
    
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

void event_queue_t::post_itc_message(itc_event_t *event) {
    int res;
    // Note: This will be atomic as long as sizeof(itc_event_t) <
    // PIPE_BUF
    res = write(this->itc_pipe[1], event, sizeof(itc_event_t));
    check("Could not post message to itc queue", res != sizeof(itc_event_t));
    
    // TODO: serialization of the whole structure isn't as fast as
    // serializing a pointer (but that would involve heap allocation,
    // and a thread-safe allocator, which is a whole other can of
    // worms). We should revisit this when it's more clear how ITC is
    // used.

    // TODO: since we now have a new, efficient ITC for passing btrees
    // around, we should get rid of this ITC system all together.
}

void event_queue_t::register_fsm(conn_fsm_t *fsm) {
    live_fsms.push_back(fsm);
    watch_resource(fsm->get_source(), eo_rdwr, fsm);
    printf("Opened socket %d\n", fsm->get_source());
}

void event_queue_t::deregister_fsm(conn_fsm_t *fsm) {
    printf("Closing socket %d\n", fsm->get_source());
    forget_resource(fsm->get_source());
    live_fsms.remove(fsm);
    curr_connections--;
    
    // TODO: there might be outstanding btrees that we're missing (if
    // we're quitting before the the operation completes). We need to
    // free the btree structure in this case (more likely the request
    // and all the btrees associated with it).
}

void event_queue_t::pull_messages_for_cpu(message_hub_t::msg_list_t *target) {
    for(int i = 0; i < parent_pool->nworkers; i++) {
        message_hub_t::msg_list_t tmp_list;
        parent_pool->workers[i]->message_hub.pull_messages(queue_id, &tmp_list);
        target->append_and_clear(&tmp_list);
    }
}

void event_queue_t::garbage_collect(void *ctx) {
    // Perform allocator gc
    tls_small_obj_alloc_accessor<alloc_t>::alloc_vector_t *allocs = tls_small_obj_alloc_accessor<alloc_t>::allocs_tl;
    if(allocs) {
        for(size_t i = 0; i < allocs->size(); i++) {
            allocs->operator[](i)->gc();
        }
    }
}

/**
 * Timer API
 */
event_queue_t::timer_t *event_queue_t::add_timer_internal(long ms, void (*callback)(void *), void *ctx, bool once) {
    
    assert(ms >= 0);
    
    timer_t *t = new timer_t();
    if (once) {
        t->once = true;
        t->next_time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS + ms;
    } else {
        t->once = false;
        t->next_time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS + ms;
        t->interval_ms = ms;
    }
    t->callback = callback;
    t->context = ctx;
    
    // This is push_front so that if new timers are scheduled from a timer callback, they will not
    // be examined until the next time process_timer_notify() is called. This isn't strictly
    // necessary because their next_time_in_ms will be in the future, so they wouldn't be triggered
    // even if the loop did get to them.
    timers.push_front(t);
    
    return t;
}

event_queue_t::timer_t *event_queue_t::add_timer(long ms, void (*cb)(void *), void *ctx) {
    return add_timer_internal(ms, cb, ctx, false);
}

event_queue_t::timer_t *event_queue_t::fire_timer_once(long ms, void (*cb)(void *), void *ctx) {
    return add_timer_internal(ms, cb, ctx, true);
}

void event_queue_t::cancel_timer(timer_t *timer) {
    timers.remove(timer);
    delete timer;
}

void event_queue_t::on_sync() {
    itc_event_t event;
    memset(&event, 0, sizeof event);
    event.event_type = iet_cache_synced;
    post_itc_message(&event);
}

#ifndef NDEBUG
void event_queue_t::deadlock_debug() {
    
    printf("=== Debug information for event queue %d ===\n", queue_id);
    
    cache->deadlock_debug();
    
    printf("\n----- Connection FSMS -----\n");
    fsm_list_t::iterator it;
    for (it = live_fsms.begin(); it != live_fsms.end(); it ++) {
        (*it).deadlock_debug();
    }
}
#endif

#ifndef NDEBUG
void deadlock_debug(void) {
    if (get_cpu_context()->event_queue) {
        get_cpu_context()->event_queue->deadlock_debug();
    } else {
        printf("You're in the wrong thread. If you're in gdb, type 'thread X' where X >= 2, and then try again.\n");
    }
}
#endif
