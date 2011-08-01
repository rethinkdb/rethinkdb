#include "arch/arch.hpp"
#include "concurrency/mutex.hpp"

void co_lock_mutex(mutex_t *mutex) {
    struct : public lock_available_callback_t, public one_waiter_cond_t {
        void on_lock_available() { pulse(); }
    } cb;
    mutex->lock(&cb);
    cb.wait_eagerly();
}
