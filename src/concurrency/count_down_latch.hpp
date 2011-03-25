#ifndef __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
#define __CONCURRENCY_COUNT_DOWN_LATCH_HPP__

#include "concurrency/rwi_lock.hpp"
#include "containers/intrusive_list.hpp"

class count_down_latch_t {
    struct waiter_t : intrusive_list_node_t<waiter_t> {
        coro_t * coro;
        waiter_t(coro_t *coro) : coro(coro) { }
    };

    intrusive_list_t<waiter_t> waiters;
    size_t count;
public:
    count_down_latch_t(size_t count) : count(count) { }
    ~count_down_latch_t() {
        guarantee(waiters.size() == 0);
    }

    void wait() {
        if (count > 0) {
            waiters.push_back(new waiter_t(coro_t::self()));

            while (count > 0)
                coro_t::wait();
        }
    }

    void count_down() {
        rassert(count > 0);
        if (--count == 0) {
            // release all waiters
            while (waiter_t *w = waiters.head()) {
                w->coro->notify();
                waiters.remove(w);
                delete w;
            }
        }
    }
private:
    DISABLE_COPYING(count_down_latch_t);
};

#endif // __CONCURRENCY_COUNT_DOWN_LATCH_HPP__
