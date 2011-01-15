
#include <string.h>
#include "event_queue.hpp"

void event_queue_base_t::signal_handler(int signum, siginfo_t *siginfo, void *uctx) {
    linux_event_callback_t *callback = (linux_event_callback_t*)siginfo->si_value.sival_ptr;
    callback->on_event(siginfo->si_overrun);
}

void event_queue_base_t::watch_signal(const sigevent *evp, linux_event_callback_t *cb) {
    // Set up a sigmask to block the signal in question
    sigset_t sigmask;
    int res = sigemptyset(&sigmask);
    guarantee_err(res == 0, "Could not create an empty signal mask");
    res = sigaddset(&sigmask, evp->sigev_signo);
    guarantee_err(res == 0, "Could not add a signal to signal mask");

    // All events are automagically blocked by thread pool, this is a
    // typical use case for epoll_pwait/poll_pwait.
    
    // Establish a handler on the signal that calls the right callback
    struct sigaction sa;
    bzero((char*)&sa, sizeof(struct sigaction));
    sa.sa_sigaction = &event_queue_base_t::signal_handler;
    sa.sa_flags = SA_SIGINFO;
    
    res = sigaction(evp->sigev_signo, &sa, NULL);
    guarantee_err(res == 0, "Could not install signal handler in event queue");
}

void event_queue_base_t::forget_signal(const sigevent *evp, linux_event_callback_t *cb) {
    // We don't support forgetting signals for now
}

