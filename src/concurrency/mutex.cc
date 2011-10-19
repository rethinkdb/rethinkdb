#include "concurrency/mutex.hpp"

#include "arch/runtime/runtime.hpp"
#include "concurrency/rwi_lock.hpp"
#include "concurrency/cond_var.hpp"

struct mutex_t::lock_request_t : public intrusive_list_node_t<mutex_t::lock_request_t>, public thread_message_t {
    lock_available_callback_t *cb;
    void on_thread_switch() {
        cb->on_lock_available();
        delete this;
    }
};

void mutex_t::lock(lock_available_callback_t *cb) {
    if (locked) {
        lock_request_t *r = new lock_request_t;
        r->cb = cb;
        waiters.push_back(r);
    } else {
        locked = true;
        cb->on_lock_available();
    }
}

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
