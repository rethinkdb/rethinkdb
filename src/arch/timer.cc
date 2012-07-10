#include <string.h>
#include <unistd.h>
#include "arch/runtime/event_queue.hpp"
#include "arch/timer.hpp"
#include "logger.hpp"
#include "arch/arch.hpp"
#include "concurrency/cond_var.hpp"

/* Timer token */
class timer_token_t : public intrusive_list_node_t<timer_token_t> {
    friend class timer_handler_t;

private:
    bool once;   // If 'false', the timer is repeating
    int64_t interval_ms;   // If a repeating timer, this is the time between 'rings'
    int64_t next_time_in_ms;   // This is the time (in ms since the server started) of the next 'ring'

    // It's unsafe to remove arbitrary timers from the list as we iterate over
    // it, so instead we set the 'deleted' flag and then remove them in a
    // controlled fashion.
    bool deleted;

    void (*callback)(void *ctx);
    void *context;
};

/* Timer implementation */
timer_handler_t::timer_handler_t(linux_event_queue_t *queue)
    : timer_provider(queue, this, TIMER_TICKS_IN_MS / 1000, (TIMER_TICKS_IN_MS % 1000) * 1000000),
      timer_ticks_since_server_startup(0)
{
    // Nothing to do here, timer_provider handles the OS
}

timer_handler_t::~timer_handler_t() {
    // Delete the registered timers
    while (timer_token_t *t = timers.head()) {
        timers.remove(t);
        if (t->deleted) {
            delete t;
        } else {
            /* This is an error. However, the best way to debug this error is to have
            the timer token leak and have Valgrind tell us where the leaked block originated
            from. So we just write a warning message. */
            logERR("Internal error: leaked timer.");
        }
    }
}

void timer_handler_t::on_timer(int nexpirations) {
    timer_ticks_since_server_startup += nexpirations;
    int64_t time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS;

    timer_token_t *p = timers.head();
    while (p) {
        timer_token_t *timer = p;

        // Increment now instead of in the header of the 'for' loop because we may
        // delete the timer we are on
        p = timers.next(p);

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

timer_token_t *timer_handler_t::add_timer_internal(int64_t ms, void (*callback)(void *ctx), void *ctx, bool once) {
    rassert(ms >= 0);

    timer_token_t *t = new timer_token_t();
    t->next_time_in_ms = timer_ticks_since_server_startup * TIMER_TICKS_IN_MS + ms;
    t->once = once;
    t->interval_ms = ms;
    t->deleted = false;
    t->callback = callback;
    t->context = ctx;

    timers.push_front(t);

    return t;
}

void timer_handler_t::cancel_timer(timer_token_t *timer) {
    timer->deleted = true;
}
