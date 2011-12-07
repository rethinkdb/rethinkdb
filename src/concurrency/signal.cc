#include "concurrency/signal.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

void signal_t::wait_lazily_ordered() {
    if (!is_pulsed()) {
        subscription_t subs(
            boost::bind(&coro_t::notify_later_ordered, coro_t::self()),
            this);
        coro_t::wait();
    }
}

void signal_t::wait_lazily_unordered() {
    if (!is_pulsed()) {
        subscription_t subs(
            boost::bind(&coro_t::notify_sometime, coro_t::self()),
            this);
        coro_t::wait();
    }
}
