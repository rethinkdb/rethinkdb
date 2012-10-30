// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef NO_EVENTFD
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
    guarantee_err(res == 0, "Could not close eventfd");
}

uint64_t eventfd_event_t::read() {
    uint64_t value;
    int res = eventfd_read(eventfd_, &value);
    guarantee_err(res == 0, "Could not read from eventfd");
    return value;
}

void eventfd_event_t::write(uint64_t value) {
    int res = eventfd_write(eventfd_, value);
    guarantee_err(res == 0, "Could not write to eventfd");
}

#endif // NO_EVENTFD

