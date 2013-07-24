// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "concurrency/cross_thread_semaphore.hpp"

#include <unistd.h>
#if defined(__linux__)
#include <sys/eventfd.h>
#else
#include <fcntl.h>
#endif

#include "concurrency/wait_any.hpp"

#if defined(__linux) && !defined(NO_EVENTFD)

typedef uint64_t semaphore_value_t;

// Implementation of cross_thread_semaphore_t for when we have eventfds
cross_thread_semaphore_t::cross_thread_semaphore_t(size_t initial_value) {
    guarantee(initial_value < 1024);

    fd_t res = eventfd(initial_value, EFD_SEMAPHORE | EFD_NONBLOCK | EFD_CLOEXEC);
    guarantee_err(res != INVALID_FD, "failed to create eventfd semaphore");
    sem.reset(res);

    thread_blockers.create(this);
}

void cross_thread_semaphore_t::unlock() {
    semaphore_value_t buffer = 1;
    ssize_t res;
    do {
        res = write(sem.get(), &buffer, sizeof(buffer));
    } while (res == -1 && errno == EINTR);
    guarantee_err(res == sizeof(buffer), "failed to write to eventfd semaphore");
}

#else

typedef char semaphore_value_t;

// Implementation of cross_thread_semaphore_t for when we don't have eventfds
cross_thread_semaphore_t::cross_thread_semaphore_t(size_t initial_value) {
    guarantee(initial_value < 1024);

    fd_t pipe_fds[2];
    int res = pipe(pipe_fds);
    guarantee_err(res == 0, "failed to create semaphore pipe");

    res = fcntl(pipe_fds[0], F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "failed to set semaphore pipe to non-blocking");

    res = fcntl(pipe_fds[1], F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "failed to set semaphore pipe to non-blocking");

    sem.reset(pipe_fds[0]);
    write_fd.reset(pipe_fds[1]);

    for (size_t i = 0; i < initial_value; ++i) {
        unlock();
    }

    thread_blockers.create(this);
}

void cross_thread_semaphore_t::unlock() {
    semaphore_value_t buffer = 1;
    ssize_t res;
    do {
        res = write(write_fd.get(), &buffer, sizeof(buffer));
    } while (res == -1 && errno == EINTR);
    guarantee_err(res == sizeof(buffer), "failed to write to eventfd semaphore");
}

#endif

bool cross_thread_semaphore_t::try_lock() {
    semaphore_value_t get_val;
    ssize_t res = 0;

    do {
        res = read(sem.get(), &get_val, sizeof(get_val));
    } while (res == -1 && errno == EINTR);

    if (res == -1) {
        guarantee_err(errno == EAGAIN || errno == EWOULDBLOCK,
                      "failed to read from eventfd semaphore");
        return false;
    }

    guarantee(res == sizeof(get_val));
    guarantee(get_val == 1);
    return true;
}

void cross_thread_semaphore_t::lock(signal_t *interruptor) {
    blocker_t *blocker = thread_blockers->get();

    if (blocker->waiting() || !try_lock()) {
        blocker->lock_blocking(interruptor);
    }
}

cross_thread_semaphore_t::blocker_t::blocker_t(cross_thread_semaphore_t *_parent) :
    parent(_parent),
    event_watcher(parent->sem.get(), &error_callback) { }

// Wait until we get a read event, then try to read again until success
void cross_thread_semaphore_t::blocker_t::lock_blocking(signal_t *interruptor) {
    lock_request_t request(&waiters);

    if (interruptor != NULL) {
        wait_interruptible(&request, interruptor);
    } else {
        request.wait();
    }

    guarantee(waiters.head() == &request);

    do {
        linux_event_watcher_t::watch_t watch(&event_watcher, poll_event_in);

        if (interruptor != NULL) {
            wait_interruptible(&watch, interruptor);
        } else {
            watch.wait();
        }
    } while(!parent->try_lock());
}

bool cross_thread_semaphore_t::blocker_t::waiting() {
    return !waiters.empty();
}

cross_thread_semaphore_t::blocker_t::lock_request_t::lock_request_t(intrusive_list_t<lock_request_t> *_list) :
    list(_list) {
    list->push_back(this);

    // If we're the only one waiting, just go
    if (list->head() == this) {
        notify();
    }
}

cross_thread_semaphore_t::blocker_t::lock_request_t::~lock_request_t() {
    list->remove(this);

    // Notify any waiters after us in the queue
    if (!list->empty()) {
        list->head()->notify();
    }
}

void cross_thread_semaphore_t::blocker_t::lock_request_t::notify() {
    pulse();
}
