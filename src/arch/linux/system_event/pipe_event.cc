
#include <unistd.h>
#include <fcntl.h>
#include "utils.hpp"
#include "pipe_event.hpp"
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
    uint64_t value;
    int res = ::read(pipefd[0], &value, sizeof(value));
    guarantee_err(res == sizeof(value), "Could not read from pipe event");
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

