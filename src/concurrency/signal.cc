#include "concurrency/signal.hpp"

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/runtime/runtime.hpp"

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

void signal_t::wait_eagerly() {
    if (!is_pulsed()) {
	subscription_t subs(
			    boost::bind(&coro_t::notify_now, coro_t::self()),
			    this);
	coro_t::wait();
    }
}

void signal_t::pulse() {
    mutex_acquisition_t acq(&mutex, false);
    rassert(!is_pulsed());
    pulsed = true;
    publisher_controller.publish(&signal_t::call, &acq);
}
