#include "event_queue.hpp"
#include <string.h>
#include "arch/linux/thread_pool.hpp"
#include "concurrency/cond_var.hpp"
#include "utils.hpp"

perfmon_duration_sampler_t pm_eventloop("eventloop", secs_to_ticks(1.0));

std::string format_poll_event(int event) {
    std::string s;
    if (event & poll_event_in) {
        s = "in";
    }
    if (event & poll_event_out) {
        if (s != "") s += " ";
        s += "out";
    }
    if (event & poll_event_err) {
        if (s != "") s += " ";
        s += "err";
    }
    if (event & poll_event_hup) {
        if (s != "") s += " ";
        s += "hup";
    }
    if (event & poll_event_rdhup) {
        if (s != "") s += " ";
        s += "rdhup";
    }
    if (s == "") s = "(none)";
    return s;
}

// TODO: I guess signum is unused because we aren't interested, and
// uctx is unused because we don't have any context data.  Is this the
// true?
void event_queue_base_t::signal_handler(UNUSED int signum, siginfo_t *siginfo, UNUSED void *uctx) {
    linux_event_callback_t *callback = (linux_event_callback_t*)siginfo->si_value.sival_ptr;
    callback->on_event(siginfo->si_overrun);
}

// TODO: Why is cb unused?
void event_queue_base_t::watch_signal(const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // All events are automagically blocked by thread pool, this is a
    // typical use case for epoll_pwait/ppoll.
    
    // Establish a handler on the signal that calls the right callback
    struct sigaction sa;
    bzero((char*)&sa, sizeof(struct sigaction));
    sa.sa_sigaction = &event_queue_base_t::signal_handler;
    sa.sa_flags = SA_SIGINFO;
    
    int res = sigaction(evp->sigev_signo, &sa, NULL);
    guarantee_err(res == 0, "Could not install signal handler in event queue");
}

// TODO: Why is cb unused?
void event_queue_base_t::forget_signal(UNUSED const sigevent *evp, UNUSED linux_event_callback_t *cb) {
    // We don't support forgetting signals for now
}

/* linux_event_watcher_t */

struct linux_event_watcher_guts_t :
    public linux_event_callback_t,
    public home_thread_mixin_t
{
    linux_event_watcher_guts_t(fd_t fd, linux_event_callback_t *eh) :
        fd(fd), error_handler(eh),
        read_handler(this), write_handler(this),
        old_mask(0)
    {
        /* Register for error events only at first */
        linux_thread_pool_t::thread->queue.watch_resource(fd, 0, this);
    }

    ~linux_event_watcher_guts_t() {
        assert_thread();        

        /* Unregister for error events */
        rassert(old_mask == 0);
        linux_thread_pool_t::thread->queue.forget_resource(fd, this);

        rassert(!read_handler.callback && !read_handler.aborter);
        rassert(!write_handler.callback && !write_handler.aborter);
    }

    fd_t fd;   // the file descriptor to watch

    linux_event_callback_t *error_handler;   // What to call if there is an error

    /* These objects handle waiting for reads and writes. Mostly they exist to be subclasses
    of signal_t::waiter_t. */
    struct waiter_t : public signal_t::waiter_t {

        waiter_t(linux_event_watcher_guts_t *p) : parent(p), callback(0), aborter(NULL) { }

        linux_event_watcher_guts_t *parent;

        boost::function<void()> callback;
        signal_t *aborter;

        void watch(const boost::function<void()> &cb, signal_t *ab) {
            rassert(!callback && !aborter);
            rassert(cb && ab);
            if (!ab->is_pulsed()) {
                callback = cb;
                aborter = ab;
                aborter->add_waiter(this);
                parent->remask();
            }
        }

        void pulse() {
            rassert(callback && aborter, "%p got a pulse() when event mask is %d", parent, parent->old_mask);
            aborter->remove_waiter(this);
            boost::function<void()> temp = callback;
            callback = 0;
            aborter = NULL;
            parent->remask();
            temp();
        }

        void on_signal_pulsed() {
            rassert(callback && aborter);
            callback = 0;
            aborter = NULL;
            parent->remask();
        }

    } read_handler, write_handler;

    int old_mask;   // So remask() knows whether we were registered with the event queue before

    int registration_thread;   // The event queue we are registered with, or -1 if we are not registered

    void on_event(int event) {

        int error_mask = poll_event_err | poll_event_hup | poll_event_rdhup;
        guarantee((event & (error_mask | old_mask)) == event, "Unexpected event received (from operating system?).");

        if (event & error_mask) {

            error_handler->on_event(event & error_mask);

            /* The error handler might have cancelled some waits, which would
             cause remask() to be run. To avoid calling waiter_t::pulse() twice,
             we "&" again with our event mask to cancel any events that we
             are no longer waiting for. */
            event &= old_mask;
        }

        if (event & poll_event_in) {
            read_handler.pulse();
        }

        if (event & poll_event_out) {
            write_handler.pulse();
        }
    }

    void watch(int event, const boost::function<void()> &cb, signal_t *ab) {
        rassert(event == poll_event_in || event == poll_event_out);
        rassert(cb);
        rassert(ab);
        assert_thread();
        waiter_t *handler = event == poll_event_in ? &read_handler : &write_handler;
        handler->watch(cb, ab);
    }

    void remask() {
        /* Change our registration with the event queue depending on what events
        we are actually waiting for. */

        int new_mask = 0;
        if (read_handler.callback) new_mask |= poll_event_in;
        if (write_handler.callback) new_mask |= poll_event_out;

        if (old_mask != new_mask) {
            linux_thread_pool_t::thread->queue.adjust_resource(fd, new_mask, this);
        }

        old_mask = new_mask;
    }
};

linux_event_watcher_t::linux_event_watcher_t(fd_t f, linux_event_callback_t *eh)
    : guts(new linux_event_watcher_guts_t(f, eh)) { }

linux_event_watcher_t::~linux_event_watcher_t() {
}

void linux_event_watcher_t::watch(int event, const boost::function<void()> &cb, signal_t *ab) {
    guts->watch(event, cb, ab);
}

