#include "concurrency/drain_semaphore.hpp"
#include "concurrency/signal.hpp"

void drain_semaphore_t::drain() {
    draining = true;
    if (refcount) {
        cond.get_signal()->wait();
        cond.reset();
    }
    draining = false;
}
