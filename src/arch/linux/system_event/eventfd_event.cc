#ifndef NO_EVENTFD

#include <fcntl.h>
#include <unistd.h>
#include "utils.hpp"
#include "eventfd_event.hpp"

eventfd_event_t::eventfd_event_t() {
    _eventfd = eventfd(0, 0);
    guarantee_err(_eventfd != -1, "Could not create eventfd");

    int res = fcntl(_eventfd, F_SETFL, O_NONBLOCK);
    guarantee_err(res == 0, "Could not make eventfd non-blocking");
}

eventfd_event_t::~eventfd_event_t() {
    int res = close(_eventfd);
    guarantee_err(res == 0, "Could not close eventfd");
}

uint64_t eventfd_event_t::read() {
    uint64_t value;
    int res = eventfd_read(_eventfd, &value);
    guarantee_err(res == 0, "Could not read from eventfd");
    return value;
}

void eventfd_event_t::write(uint64_t value) {
    int res = eventfd_write(_eventfd, value);
    guarantee_err(res == 0, "Could not write to eventfd");
}

#endif // NO_EVENTFD

