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

void co_read(direct_file_t *file, size_t offset, size_t length, void *buf, file_account_t *account) {
    io_coroutine_adapter_t adapter;
    file->read_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

void co_write(direct_file_t *file, size_t offset, size_t length, void *buf, file_account_t *account) {
    io_coroutine_adapter_t adapter;
    file->write_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}
