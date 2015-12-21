// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "arch/arch.hpp"

#include "arch/runtime/coroutines.hpp"
#include "logger.hpp"
#include "containers/scoped.hpp"

struct io_coroutine_adapter_t : public iocallback_t {
    coro_t *cont;
    io_coroutine_adapter_t() : cont(coro_t::self()) { }
    void on_io_complete() {
        cont->notify_later_ordered();
    }
};

void co_read(file_t *file, int64_t offset, size_t length, void *buf, file_account_t *account) {
    io_coroutine_adapter_t adapter;
    file->read_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

void co_write(file_t *file, int64_t offset, size_t length, void *buf,
              file_account_t *account, file_t::wrap_in_datasyncs_t wrap_in_datasyncs) {
    io_coroutine_adapter_t adapter;
    file->write_async(offset, length, buf, account, &adapter, wrap_in_datasyncs);
    coro_t::wait();
}
