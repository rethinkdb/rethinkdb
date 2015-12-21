// Copyright 2010-2013 RethinkDB, all rights reserved.
#if defined(__linux) && !defined(NO_EVENTFD)

#include "arch/runtime/system_event/eventfd_event.hpp"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "errors.hpp"
#include "arch/runtime/system_event/eventfd.hpp"

eventfd_event_t::eventfd_event_t() {
    eventfd_ = eventfd(0, 0);
    guarantee_err(eventfd_ != -1, "Could not create eventfd");

    int res = fcntl(eventfd_, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make eventfd non-blocking");
}

eventfd_event_t::~eventfd_event_t() {
    int res = close(eventfd_);
    guarantee_err(res == 0 || get_errno() == EINTR, "Could not close eventfd");
}

void eventfd_event_t::consume_wakey_wakeys() {
    // We don't care how many pings we consumed from the eventfd.
    uint64_t value;
    ssize_t res;
    do {
        res = ::read(eventfd_, &value, sizeof(value));
    } while (res == -1 && get_errno() == EINTR);

    if (res == -1) {
        guarantee_err(get_errno() == EAGAIN || get_errno() == EWOULDBLOCK, "Could not read from eventfd.");
    } else {
        guarantee(res == sizeof(value), "Trouble reading from an eventfd: read %zd bytes", res);
    }
}

void eventfd_event_t::wakey_wakey() {
    const uint64_t value = 1;
    ssize_t res;
    do {
        res = ::write(eventfd_, &value, sizeof(value));
    } while (res == -1 && get_errno() == EINTR);

    guarantee_err(res != -1, "Could not write to an eventfd.");
    guarantee(res == sizeof(value), "Somehow completed a partial write to an eventfd.");
}

#endif
