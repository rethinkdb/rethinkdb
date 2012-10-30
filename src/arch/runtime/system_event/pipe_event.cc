// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/system_event/pipe_event.hpp"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "utils.hpp"
#include "logger.hpp"

pipe_event_t::pipe_event_t() {
    int res = pipe(pipefd);
    guarantee_err(res == 0, "Could not create system event pipe");

    res = fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make pipe[0] non-blocking");

    res = fcntl(pipefd[1], F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make pipe[1] non-blocking");
}

pipe_event_t::~pipe_event_t() {
    int res = close(pipefd[0]);
    guarantee_err(res == 0, "Could not close pipefd[0]");

    res = close(pipefd[1]);
    guarantee_err(res == 0, "Could not close pipefd[1]");
}

uint64_t pipe_event_t::read() {
    // TODO: in practice, we might not be able to read eight bytes at
    // once, so we should have a buffer in this class to manage
    // it. However, we never really use events for their counters, so
    // fuck it for now.
    int res;
    uint64_t value = 0, _temp;
    do {
        _temp = 0;
        res = ::read(pipefd[0], &_temp, sizeof(value));
        if(res == -1 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
            crash("Cannot read from pipe-based event");
        } else {
            value += _temp;
        }
    } while(res != -1);

    return value;
}

void pipe_event_t::write(uint64_t value) {
    // Under some pathological cases, writes may fail (if the pipe is
    // full). Ideally, in this case we'd register the write end of the
    // pipe with the message queue, catch the event, and write
    // whatever's remanining. However, this is code to support VERY
    // legacy kernels, so fuck it. We'll just log it, cross our
    // fingers, and hope it works. If we can't write, it probably
    // don't matter to Jesus anyway because the other side of the pipe
    // will be awoken by all the other crap in it.
    int res = ::write(pipefd[1], &value, sizeof(value));
    if(res != sizeof(value)) {
        logERR("Can't write to system event pipe");
    }
}

