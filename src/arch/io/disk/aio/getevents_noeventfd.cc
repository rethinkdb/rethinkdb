#include "arch/io/disk/aio/getevents_noeventfd.hpp"

#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <signal.h>

#include <algorithm>

#include "config/args.hpp"
#include "utils.hpp"
#include "logger.hpp"

#define IO_SHUTDOWN_NOTIFY_SIGNAL     (SIGRTMIN + 4)

/* Async IO scheduler - the io_getevents part */
void shutdown_signal_handler(UNUSED int signum, UNUSED siginfo_t *siginfo, UNUSED void *uctx) {
    // Don't do shit...
}

void* linux_aio_getevents_noeventfd_t::io_event_loop(void *arg) {
    // Unblock a signal that will tell us to shutdown
    sigset_t sigmask;
    int res = sigemptyset(&sigmask);
    guaranteef_err(res == 0, "Could not get a full sigmask");
    res = sigaddset(&sigmask, IO_SHUTDOWN_NOTIFY_SIGNAL);
    guaranteef_err(res == 0, "Could not add a signal to the set");
    res = pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
    guaranteef_err(res == 0, "Could not block signal");

    linux_aio_getevents_noeventfd_t *parent = static_cast<linux_aio_getevents_noeventfd_t *>(arg);

    io_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];

    do {
        /* XXX Notice: The use of a timeout here is necessary here to prevent a
         * terrible race condition (Issue #377). The problem is that we rely on
         * the signal to get us out of io_getevents call so if we get it before
         * we make the call nothing gets us out. */
        timespec timeout = { 1, 0 };  // A 1 second timeout
        int nevents = io_getevents(parent->parent->aio_context.id, 1, MAX_IO_EVENT_PROCESSING_BATCH_SIZE,
                                   events, &timeout);
        if (nevents == -EINTR || nevents == 0) {
            if(parent->shutting_down)
                break;
            else
                continue;
        }
        guarantee_xerr(nevents >= 1, -nevents, "Waiting for AIO event failed");

        // Push the events on the parent thread's list
        int lockres = pthread_mutex_lock(&parent->io_mutex);
        guaranteef(lockres == 0, "Could not lock io mutex");

        // TODO: WTF is this shit?  This is a complete bullshit way to send this information between threads.
        std::copy(events, events + nevents, std::back_inserter(parent->io_events));

        res = pthread_mutex_unlock(&parent->io_mutex);
        guaranteef(res == 0, "Could not unlock io mutex");

        // Notify the parent's thread it's got events
        parent->aio_notify_event.write(nevents);

    } while (true);

    return NULL;
}

/* Async IO scheduler - the poll/epoll part */
linux_aio_getevents_noeventfd_t::linux_aio_getevents_noeventfd_t(linux_diskmgr_aio_t *_parent)
    : parent(_parent), shutting_down(false)
{
    int res;

    // Watch aio notify event
    parent->queue->watch_resource(aio_notify_event.get_notify_fd(), poll_event_in, this);

    // Create the mutex to sync IO and poll threads
    res = pthread_mutex_init(&io_mutex, NULL);
    guaranteef(res == 0, "Could not create io mutex");

    // Start the second thread to watch IO
    res = pthread_create(&io_thread, NULL, &linux_aio_getevents_noeventfd_t::io_event_loop, this);
    guaranteef(res == 0, "Could not create io thread");
}

linux_aio_getevents_noeventfd_t::~linux_aio_getevents_noeventfd_t()
{
    int res;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = &shutdown_signal_handler;
    sa.sa_flags = SA_SIGINFO;
    res = sigaction(IO_SHUTDOWN_NOTIFY_SIGNAL, &sa, NULL);
    guaranteef_err(res == 0, "Could not install shutdown signal handler in the IO thread");

    shutting_down = true;

    res = pthread_kill(io_thread, IO_SHUTDOWN_NOTIFY_SIGNAL);
    guaranteef(res == 0, "Could not notify the io thread about shutdown");

    res = pthread_join(io_thread, NULL);
    guaranteef(res == 0, "Could not join the io thread");

    // Destroy the mutex to sync IO and poll threads
    res = pthread_mutex_destroy(&io_mutex);
    guaranteef(res == 0, "Could not destroy io mutex");

    parent->queue->forget_resource(aio_notify_event.get_notify_fd(), this);
}

void linux_aio_getevents_noeventfd_t::prep(UNUSED iocb *req) {
    /* Do nothing. aio_prep() exists for the benefit of linux_aio_getevents_eventfd_t. */
}

void linux_aio_getevents_noeventfd_t::on_event(int event_mask) {

    if (event_mask != poll_event_in) {
        logERR("Unexpected event mask: %d", event_mask);
    }

    // Make sure we flush the pipe
    aio_notify_event.read();

    // Notify code that waits on the events
    int res = pthread_mutex_lock(&io_mutex);
    guaranteef(res == 0, "Could not lock io mutex");

    for(unsigned int i = 0; i < io_events.size(); i++) {
        parent->aio_notify(reinterpret_cast<iocb *>(io_events[i].obj), io_events[i].res);
    }
    io_events.clear();

    res = pthread_mutex_unlock(&io_mutex);
    guaranteef(res == 0, "Could not unlock io mutex");
}

