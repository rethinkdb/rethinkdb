#include <sys/timerfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "event_queue.hpp"
#include "timer.hpp"

linux_timer_handler_t::linux_timer_handler_t(linux_event_queue_t *queue)
    : queue(queue)
{
    int res;
    
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
    
    queue->watch_resource(timer_fd, EPOLLET|EPOLLIN, this);
}

linux_timer_handler_t::~linux_timer_handler_t() {
    
    int res;
    
    // Delete the registered timers
    // TODO: We should instead assert there are no timers.
    while (linux_timer_token_t *t = timers.head()) {
        timers.remove(t);
        delete t;
    }
    
    res = close(timer_fd);
    check("Could not close the timer.", res != 0);
}

void linux_timer_handler_t::on_epoll(int events) {
    
    int res;
    eventfd_t nexpirations;
    
    res = eventfd_read(timer_fd, &nexpirations);
    check("Could not read timer_fd value", res != 0);
    
    timer_ticks_since_server_startup += nexpirations;
    long time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS;

    intrusive_list_t<linux_timer_token_t>::iterator t;
    for (t = timers.begin(); t != timers.end(); ) {
        
        linux_timer_token_t *timer = &*t;
        
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
    
linux_timer_token_t *linux_timer_handler_t::add_timer_internal(long ms, void (*callback)(void *ctx), void *ctx, bool once) {

    assert(ms >= 0);
    
    linux_timer_token_t *t = new linux_timer_token_t();
    t->next_time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS + ms;
    t->once = once;
    if (once) t->interval_ms = ms;
    t->deleted = false;
    t->callback = callback;
    t->context = ctx;
    
    timers.push_front(t);
    
    return t;
}

void linux_timer_handler_t::cancel_timer(linux_timer_token_t *timer) {

    timer->deleted = true;
}

