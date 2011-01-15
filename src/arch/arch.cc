#include "arch/arch.hpp"

struct io_coroutine_adapter_t : public iocallback_t {
    coro_t *cont;
    io_coroutine_adapter_t() : cont(coro_t::self()) { }
    void on_io_complete() {
        cont->notify();
    }
};

void co_read(direct_file_t *file, size_t offset, size_t length, void *buf) {
    io_coroutine_adapter_t adapter;
    file->read_async(offset, length, buf, &adapter);
    coro_t::wait();
}

void co_write(direct_file_t *file, size_t offset, size_t length, void *buf) {
    io_coroutine_adapter_t adapter;
    file->write_async(offset, length, buf, &adapter);
    coro_t::wait();
}

struct write_external_coroutine_adapter_t : public net_conn_write_external_callback_t {
    coro_t *cont;
    net_conn_write_external_result_t answer;
    write_external_coroutine_adapter_t() : cont(coro_t::self()) { }
    virtual void on_net_conn_close() {
        answer.result = net_conn_write_external_result_t::closed;
        cont->notify();
    }
    virtual void on_net_conn_write_external() {
        answer.result = net_conn_write_external_result_t::success;
        cont->notify();
    }
};

net_conn_write_external_result_t co_write_external(net_conn_t *conn, const void *buf, size_t size) {
    write_external_coroutine_adapter_t cb;
    conn->write_external(buf, size, &cb);
    coro_t::wait();
    return cb.answer;
}

struct read_external_coroutine_adapter_t : public net_conn_read_external_callback_t {
    coro_t *cont;
    net_conn_read_external_result_t answer;
    read_external_coroutine_adapter_t() : cont(coro_t::self()) { }
    virtual void on_net_conn_close() {
        answer.result = net_conn_read_external_result_t::closed;
        cont->notify();
    }
    virtual void on_net_conn_read_external() {
        answer.result = net_conn_read_external_result_t::success;
        cont->notify();
    }
};

net_conn_read_external_result_t co_read_external(net_conn_t *conn, void *buf, size_t size) {
    read_external_coroutine_adapter_t cb;
    conn->read_external(buf, size, &cb);
    coro_t::wait();
    return cb.answer;
}
