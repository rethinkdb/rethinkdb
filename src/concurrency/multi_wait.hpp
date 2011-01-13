#ifndef __CONCURRENCY_MULTI_WAIT_HPP__
#define __CONCURRENCY_MULTI_WAIT_HPP__

#include "arch/arch.hpp"

struct multi_wait_t {

private:
    coro_t *waiter;
    unsigned int notifications_left;
    DISABLE_COPYING(multi_wait_t);

public:
    multi_wait_t(unsigned int notifications_left)
        : waiter(coro_t::self()), notifications_left(notifications_left) {
        assert(notifications_left > 0);
    }
    void notify() {
        notifications_left--;
        if (notifications_left == 0) {
            waiter->notify();
            delete this;
        }
    }
};

#endif /* __CONCURRENCY_MULTI_WAIT_HPP__ */
