#ifndef __CONCURRENCY_SIDE_CORO_HPP__
#define __CONCURRENCY_SIDE_CORO_HPP__

#include "errors.hpp"
#include <boost/function.hpp>

#include "concurrency/cond_var.hpp"

/* Sometimes there is some auxiliary loop that needs to be running in a separate
coroutine for the entire lifetime of an object. `side_coro_handler_t` takes care of
starting up and stopping such loops. Pass it a function; it will call that function with
a signal_t*, which it will signal when it's time to shut down. It takes care of waiting
to make sure that the side coroutine exits before `~side_coro_handler_t()` returns. */

class side_coro_handler_t {
public:
    side_coro_handler_t(const boost::function<void(signal_t *)> &f);
    ~side_coro_handler_t() {
        debugf("~side_coro_handler_t begin\n");
        stop_cond.pulse();
        debugf("~side_coro_handler_t stop_cond pulsed.\n");
        done_cond.wait();
        debugf("~side_coro_handler_t done_cond wait finished.\n");
    }

private:
    boost::function<void(signal_t *)> fun;
    cond_t stop_cond, done_cond;
    void run();

};

#endif /* __CONCURRENCY_SIDE_CORO_HPP__ */
