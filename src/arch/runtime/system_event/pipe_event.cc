// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef _WIN32

#include "arch/runtime/system_event/pipe_event.hpp"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "utils.hpp"
#include "logger.hpp"

pipe_event_t::pipe_event_t() {
    int pipefd[2];
    int res = pipe(pipefd);
    guarantee_err(res == 0, "Could not create system event pipe");

    read_fd_ = pipefd[0];
    write_fd_ = pipefd[1];

    res = fcntl(read_fd_, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make the notify pipe's read end non-blocking");

    res = fcntl(write_fd_, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make the notify pipe's write end non-blocking");
}

pipe_event_t::~pipe_event_t() {
    int res = close(read_fd_);
    guarantee_err(res == 0 || get_errno() == EINTR, "Could not close notify pipe's reading end");

    res = close(write_fd_);
    guarantee_err(res == 0 || get_errno() == EINTR, "Could not close notify pipe's writing end");
}

void pipe_event_t::consume_wakey_wakeys() {
    char buf[512];
    ssize_t res;
    do {
        do {
            res = read(read_fd_, buf, sizeof(buf));
        } while (res == -1 && get_errno() == EINTR);
    } while (res == sizeof(buf));

    if (res == -1) {
        // We normally expect a short read count, but if there were an exact multiple of sizeof(buf)
        // notifications on the pipe, we would get EAGAIN when we try to read again and there's
        // nothing on the pipe.
        guarantee_err(get_errno() == EAGAIN || get_errno() == EWOULDBLOCK, "Could not read from notification pipe");
    }
}

void pipe_event_t::wakey_wakey() {
    char buf[1] = { 0 };
    ssize_t res;
    do {
        res = write(write_fd_, buf, sizeof(buf));
    } while (res == -1 && get_errno() == EINTR);

    if (res == -1) {
        // EAGAIN and EWOULDBLOCK are okay because that means the pipe is full, and the recipient
        // already has pending notifications anyway.
        guarantee_err(get_errno() == EAGAIN || get_errno() == EWOULDBLOCK, "Could not write to notification pipe");
    }
}

#endif
