// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/arch.hpp"
#include "arch/runtime/runtime.hpp"

struct io_coroutine_adapter_t : public iocallback_t {
    coro_t *cont;
    io_coroutine_adapter_t() : cont(coro_t::self()) { }
    void on_io_complete() {
        cont->notify_later_ordered();
    }
};

void co_read(file_t *file, size_t offset, size_t length, void *buf, file_account_t *account) {
    io_coroutine_adapter_t adapter;
    file->read_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

void co_write(file_t *file, size_t offset, size_t length, void *buf, file_account_t *account) {
    io_coroutine_adapter_t adapter;
    file->write_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

timer_token_t *add_timer(int64_t ms, timer_callback_t *callback) {
    return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, false);
}

timer_token_t *fire_timer_once(int64_t ms, timer_callback_t *callback) {
    return linux_thread_pool_t::thread->timer_handler.add_timer_internal(ms, callback, true);
}

void cancel_timer(timer_token_t *timer) {
    linux_thread_pool_t::thread->timer_handler.cancel_timer(timer);
}
