#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include "linux_io_calls_no_eventfd.hpp"
#include "arch/linux/arch.hpp"
#include "config/args.hpp"
#include "utils2.hpp"
#include "logger.hpp"

#define IO_SHUTDOWN_NOTIFY_SIGNAL     (SIGRTMIN + 4)

/* Async IO scheduler - the io_getevents part */
void shutdown_signal_handler(int signum, siginfo_t *siginfo, void *uctx) {
    // Don't do shit...
}

void* io_event_loop(void *arg) {
    linux_io_calls_no_eventfd_t *parent = (linux_io_calls_no_eventfd_t*)arg;

    io_event events[MAX_IO_EVENT_PROCESSING_BATCH_SIZE];

    do {
        int nevents = io_getevents(parent->aio_context, 1, MAX_IO_EVENT_PROCESSING_BATCH_SIZE,
                                   events, NULL);
        if(nevents == -EINTR) {
            if(parent->shutting_down)
                break;
            else
                continue;
        }
        guarantee_xerr(nevents >= 1, -nevents, "Waiting for AIO event failed");

        // Push the events on the parent thread's list
        int res = pthread_mutex_lock(&parent->io_mutex);
        guarantee(res == 0, "Could not lock io mutex");

        std::copy(events, events + nevents, std::back_inserter(parent->io_events));
        
        res = pthread_mutex_unlock(&parent->io_mutex);
        guarantee(res == 0, "Could not unlock io mutex");

        // Notify the parent's thread it's got events
        parent->aio_notify_event.write(nevents);
        
    } while (true);

    return NULL;
}

/* Async IO scheduler - the poll/epoll part */
linux_io_calls_no_eventfd_t::linux_io_calls_no_eventfd_t(linux_event_queue_t *queue)
    : linux_io_calls_base_t(queue), shutting_down(false)
{
    int res;
    
    // Watch aio notify event
    queue->watch_resource(aio_notify_event.get_notify_fd(), poll_event_in, this);

    // Create the mutex to sync IO and poll threads
    res = pthread_mutex_init(&io_mutex, NULL);
    guarantee(res == 0, "Could not create io mutex");

    // Start the second thread to watch IO
    res = pthread_create(&io_thread, NULL, io_event_loop, this);
    guarantee(res == 0, "Could not create io thread");
}

linux_io_calls_no_eventfd_t::~linux_io_calls_no_eventfd_t()
{
    int res;
    
    // First rule of fight club - we can't shut down until our IO
    // thread does. So notify it and wait...
    struct sigaction sa;
    bzero((char*)&sa, sizeof(struct sigaction));
    sa.sa_sigaction = &shutdown_signal_handler;
    sa.sa_flags = SA_SIGINFO;
    res = sigaction(IO_SHUTDOWN_NOTIFY_SIGNAL, &sa, NULL);
    guarantee_err(res == 0, "Could not install shutdown signal handler in the IO thread");

    shutting_down = true;
    
    res = pthread_kill(io_thread, IO_SHUTDOWN_NOTIFY_SIGNAL);
    guarantee(res == 0, "Could not notify the io thread about shutdown");
    
    res = pthread_join(io_thread, NULL);
    guarantee(res == 0, "Could not join the io thread");
    
    // Destroy the mutex to sync IO and poll threads
    res = pthread_mutex_destroy(&io_mutex);
    guarantee(res == 0, "Could not destroy io mutex");
}

void linux_io_calls_no_eventfd_t::on_event(int event_mask) {

    if (event_mask != poll_event_in) {
        logERR("Unexpected event mask: %d\n", event_mask);
    }

    // Make sure we flush the pipe
    aio_notify_event.read();

    // Notify code that waits on the events
    int res = pthread_mutex_lock(&io_mutex);
    guarantee(res == 0, "Could not lock io mutex");

        
    for(unsigned int i = 0; i < io_events.size(); i++) {
        aio_notify((iocb*)io_events[i].obj, io_events[i].res);
    }
    io_events.clear();

    res = pthread_mutex_unlock(&io_mutex);
    guarantee(res == 0, "Could not unlock io mutex");
}

