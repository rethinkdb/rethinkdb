#include "arch/arch.hpp"
#include "concurrency/mutex.hpp"

struct co_lock_waiter_t : public lock_available_callback_t {
    coro_t *self;
    co_lock_waiter_t() : self(coro_t::self()) { }
    virtual void on_lock_available() { self->notify(); }
};

void co_lock_mutex(mutex_t *mutex) {
    co_lock_waiter_t cb;
    mutex->lock(&cb);
    coro_t::wait();
}
