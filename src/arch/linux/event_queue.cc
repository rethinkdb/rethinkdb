
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
#include "config/alloc.hpp"
#include "utils.hpp"
#include "arch/linux/io.hpp"
#include "arch/linux/event_queue.hpp"
#include "arch/linux/thread_pool.hpp"

// TODO: report event queue statistics.

linux_event_queue_t::linux_event_queue_t(linux_thread_pool_t *pool, int current_cpu)
    : message_hub(pool, current_cpu), iosys(this)
{
    int res;

    // Create aio context
    
    aio_context = 0;
    res = io_setup(MAX_CONCURRENT_IO_REQUESTS, &aio_context);
    check("Could not setup aio context", res != 0);
    
    // Create a poll fd
    
    epoll_fd = epoll_create1(0);
    check("Could not create epoll fd", epoll_fd == -1);

    // Create aio notify fd
    
    aio_notify_fd = eventfd(0, 0);
    check("Could not create aio notification fd", aio_notify_fd == -1);

    res = fcntl(aio_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make aio notify fd non-blocking", res != 0);

    watch_resource(aio_notify_fd, EPOLLET|EPOLLIN, (linux_epoll_callback_t*)(intptr_t)aio_notify_fd);

    // Create notify fd for other cores that send work to us
    
    core_notify_fd = eventfd(0, 0);
    check("Could not create core notification fd", core_notify_fd == -1);

    res = fcntl(core_notify_fd, F_SETFL, O_NONBLOCK);
    check("Could not make core notify fd non-blocking", res != 0);

    watch_resource(core_notify_fd, EPOLLET|EPOLLIN, (linux_epoll_callback_t*)(intptr_t)core_notify_fd);
    
    // Create the timer
    
    timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    check("Could not create timer", timer_fd < 0);

    res = fcntl(timer_fd, F_SETFL, O_NONBLOCK);
    check("Could not make timer non-blocking", res != 0);

    itimerspec timer_spec;
    bzero(&timer_spec, sizeof(timer_spec));
    timer_spec.it_value.tv_sec = timer_spec.it_interval.tv_sec = TIMER_TICKS_IN_MS / 1000;
    timer_spec.it_value.tv_nsec = timer_spec.it_interval.tv_nsec = (TIMER_TICKS_IN_MS % 1000) * 1000000L;
    res = timerfd_settime(timer_fd, 0, &timer_spec, NULL);
    check("Could not arm the timer.", res != 0);
    
    timer_ticks_since_server_startup = 0;
    
    watch_resource(timer_fd, EPOLLET|EPOLLIN, (linux_epoll_callback_t*)(intptr_t)timer_fd);
    
    // Initialize garbage collection timer
    
    add_timer_internal(ALLOC_GC_INTERVAL_MS, &garbage_collect, NULL, false);
}

void linux_event_queue_t::process_aio_notify() {
    
    int res, nevents;
    eventfd_t nevents_total;
    
    res = eventfd_read(aio_notify_fd, &nevents_total);
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
        nevents = io_getevents(aio_context, 0,
                               std::min((int)nevents_total, MAX_IO_EVENT_PROCESSING_BATCH_SIZE),
                               events, NULL);
        check("Waiting for AIO event failed", nevents < 1);
        
        // Process the events
        for(int i = 0; i < nevents; i++) {
            iosys.aio_notify((iocb*)events[i].obj, events[i].res);
        }
        nevents_total -= nevents;
        
    } while (nevents_total > 0);
}

void linux_event_queue_t::process_timer_notify() {
    int res;
    eventfd_t nexpirations;
    
    res = eventfd_read(timer_fd, &nexpirations);
    check("Could not read timer_fd value", res != 0);
    
    timer_ticks_since_server_startup += nexpirations;
    long time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS;

    intrusive_list_t<timer_t>::iterator t;
    for (t = timers.begin(); t != timers.end(); ) {
        
        timer_t *timer = &*t;
        
        // Increment now instead of in the header of the 'for' loop because we may
        // delete the timer we are on
        t++;
        
        if (!timer->deleted && time_in_ms > timer->next_time_in_ms) {
            
            // Note that a repeating timer may have "expired" multiple times since the last time
            // process_timer_notify() was called. However, everything that uses the timer mechanism
            // right now works better if the timer's callback only happens once. Perhaps there
            // should be a flag on the timer that determines whether to call the timer's callback
            // more than once or not.
            
            timer->callback(timer->context);
            
            if (timer->once) {
                cancel_timer(timer);
            } else {
                timer->next_time_in_ms = time_in_ms + timer->interval_ms;
            }
        }
        
        if (timer->deleted) {
            timers.remove(timer);
            delete timer;
        }
    }
}

void linux_event_queue_t::run() {
    
    int res;
    
    // Now, start the loop
    do_shut_down = false;
    while (!do_shut_down) {
    
        // Grab the events from the kernel!
        res = epoll_wait(epoll_fd, events, MAX_IO_EVENT_PROCESSING_BATCH_SIZE, -1);
        
        // epoll_wait might return with EINTR in some cases (in
        // particular under GDB), we just need to retry.
        if(res == -1 && errno == EINTR) {
            continue;
        }
        check("Waiting for epoll events failed", res == -1);

        // nevents might be used by forget_resource during the loop
        nevents = res;

        // TODO: instead of processing the events immediately, we
        // might want to queue them up and then process the queue in
        // bursts. This might reduce response time but increase
        // overall throughput because it will minimize cache faults
        // associated with instructions as well as data structures
        // (see section 7 [CPU scheduling] in B-tree Indexes and CPU
        // Caches by Goetz Graege and Pre-Ake Larson).

        for (int i = 0; i < res; i++) {
            resource_t source = reinterpret_cast<intptr_t>(events[i].data.ptr);
            if (source == -1) {
                // The event was probably queued for a resource that's
                // been destroyed, so forget_resource is kindly
                // notifying us to skip it.
                continue;
            } else if (source == aio_notify_fd) {
                process_aio_notify();
            } else if (source == timer_fd) {
                process_timer_notify();
            } else if (source == core_notify_fd) {
                // Great, we needed to wake up to process batch jobs
                // from other cores, no need to do anything here.
            } else {
                linux_epoll_callback_t *cb = (linux_epoll_callback_t*)events[i].data.ptr;
                cb->on_epoll(events[i].events);
            }
        }

        // All done with OS event processing
        nevents = 0;

        // We're done with the current batch of events, process cross
        // CPU requests
        linux_message_hub_t::msg_list_t cpu_requests;
        for(int i = 0; i < message_hub.thread_pool->n_threads; i++) {
            message_hub.thread_pool->queues[i]->message_hub.pull_messages(message_hub.current_cpu, &cpu_requests);
        }
        while (linux_cpu_message_t *m = cpu_requests.head()) {
            cpu_requests.remove(m);
            m->on_cpu_switch();
        }

        // Push the messages we collected in this batch for other CPUs
        message_hub.push_messages();
        
    }

    // Delete the registered timers
    // TODO: We should instead assert there are no timers.
    while(timer_t *t = timers.head()) {
        timers.remove(t);
        delete t;
    }
}

linux_event_queue_t::~linux_event_queue_t()
{
    int res;
    
    res = close(epoll_fd);
    check("Could not close epoll_fd", res != 0);
    
    res = close(core_notify_fd);
    check("Could not close core_notify_fd", res != 0);
    
    res = close(aio_notify_fd);
    check("Could not close aio_notify_fd", res != 0);
    
    res = io_destroy(aio_context);
    check("Could not destroy aio_context", res != 0);
    
    res = close(timer_fd);
    check("Could not close the timer.", res != 0);
}

void linux_event_queue_t::watch_resource(resource_t resource, int watch_mode, linux_epoll_callback_t *cb) {

    assert(cb);
    epoll_event event;
    
    event.events = watch_mode;
    event.data.ptr = (void*)cb;
    
    int res = epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, resource, &event);
    check("Could not pass socket to worker", res != 0);
}

void linux_event_queue_t::forget_resource(resource_t resource, linux_epoll_callback_t *cb) {
    epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = NULL;
    int res = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, resource, &event);
    check("Couldn't remove socket from watching", res != 0);

    // Go through the queue of messages in the current poll cycle and
    // clean out the ones that are referencing the resource we're
    // being asked to forget.
    if (cb) {
        for (int i = 0; i < nevents; i++) {
            void *ptr = events[i].data.ptr;
            if (ptr == (void*)cb) {
                events[i].data.fd = -1;
            }
        }
    }
}

void linux_event_queue_t::garbage_collect(void *ctx) {
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
linux_event_queue_t::timer_t *linux_event_queue_t::add_timer_internal(long ms, void (*callback)(void *), void *ctx, bool once) {
    
    assert(ms >= 0);
    
    timer_t *t = new timer_t();
    t->next_time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS + ms;
    t->once = once;
    if (once) t->interval_ms = ms;
    t->deleted = false;
    t->callback = callback;
    t->context = ctx;
    
    timers.push_front(t);
    
    return t;
}

void linux_event_queue_t::cancel_timer(timer_t *timer) {
    timer->deleted = true;
}
