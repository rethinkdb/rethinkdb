#include "arch/arch.hpp"

struct io_coroutine_adapter_t : public iocallback_t {
    coro_t *cont;
    io_coroutine_adapter_t() : cont(coro_t::self()) { }
    void on_io_complete() {
        cont->notify();
    }
};

void co_read(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account) {
    io_coroutine_adapter_t adapter;
    file->read_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}

void co_write(direct_file_t *file, size_t offset, size_t length, void *buf, direct_file_t::account_t *account) {
    io_coroutine_adapter_t adapter;
    file->write_async(offset, length, buf, account, &adapter);
    coro_t::wait();
}
