#ifndef CONCURRENCY_INTERRUPTOR_HPP_
#define CONCURRENCY_INTERRUPTOR_HPP_

#include <stdexcept>

#include "errors.hpp"

class signal_t;

/* General exception to be thrown when some process is interrupted. It's in
`utils.hpp` because I can't think where else to put it */
class interrupted_exc_t : public std::exception {
public:
    const char *what() const throw () {
        return "interrupted";
    }
};

/* Waits for the first signal to become pulsed. If the second signal becomes
pulsed, stops waiting and throws `interrupted_exc_t`. */
void wait_interruptible(const signal_t *signal, const signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

void wait_timeout(const signal_t *signal, int64_t ms) {
    signal_timer_t timer;
    timer.start(ms);
    try {
        wait_interruptible(signal, &timer);
    } catch (const interrupted_exc_t &e) {
        crash();
    }
}

#endif  // CONCURRENCY_INTERRUPTOR_HPP_
