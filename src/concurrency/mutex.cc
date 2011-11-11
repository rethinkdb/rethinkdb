#include "arch/runtime/runtime.hpp"
#include "concurrency/mutex.hpp"
#include "concurrency/cond_var.hpp"

void co_lock_mutex(mutex_t *mutex) {
    struct : public lock_available_callback_t, public one_waiter_cond_t {
        void on_lock_available() { pulse(); }
    } cb;
    mutex->lock(&cb);
    cb.wait_eagerly();
}

void mutex_t::unlock(bool eager) {
    rassert(locked);
    if (lock_request_t *h = waiters.head()) {
        waiters.remove(h);
        if (eager) {
            h->on_thread_switch();
        } else {
            call_later_on_this_thread(h);
        }
    } else {
        locked = false;
    }
}
