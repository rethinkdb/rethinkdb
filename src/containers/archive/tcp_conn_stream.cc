// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "containers/archive/tcp_conn_stream.hpp"

#include "arch/io/network.hpp"

tcp_conn_stream_t::tcp_conn_stream_t(const ip_address_t &host, int port, signal_t *interruptor, int local_port)
    : conn_(new tcp_conn_t(host, port, interruptor, local_port)) { }

tcp_conn_stream_t::tcp_conn_stream_t(tcp_conn_t *conn) : conn_(conn) {
    rassert(conn_ != NULL);
}

tcp_conn_stream_t::~tcp_conn_stream_t() {
    delete conn_;
}

int64_t tcp_conn_stream_t::read(void *p, int64_t n) {
    // Returns the number of bytes read, or 0 upon EOF, -1 upon error.
    // Right now this function cannot "error".
    try {
        cond_t non_closer;
        size_t result = conn_->read_some(p, n, &non_closer);
        rassert(result > 0);
        rassert(int64_t(result) <= n);
        return result;
    } catch (const tcp_conn_read_closed_exc_t &) {
        return 0;
    }
}

int64_t tcp_conn_stream_t::write(const void *p, int64_t n) {
    try {
        // write writes everything or throws an exception.
        cond_t non_closer;
        conn_->write(p, n, &non_closer);
        return n;
    } catch (const tcp_conn_write_closed_exc_t &) {
        return -1;
    }
}

void tcp_conn_stream_t::rethread(threadnum_t new_thread) {
    conn_->rethread(new_thread);
}

threadnum_t tcp_conn_stream_t::home_thread() const {
    return conn_->home_thread();
}

void tcp_conn_stream_t::shutdown_read() {
    conn_->shutdown_read();
}

void tcp_conn_stream_t::shutdown_write() {
    conn_->shutdown_write();
}

bool tcp_conn_stream_t::is_read_open() {
    return conn_->is_read_open();
}

bool tcp_conn_stream_t::is_write_open() {
    return conn_->is_write_open();
}


keepalive_tcp_conn_stream_t::keepalive_tcp_conn_stream_t(const ip_address_t &host, int port, signal_t *interruptor, int local_port) :
    tcp_conn_stream_t(host, port, interruptor, local_port),
    keepalive_callback(NULL) { }

keepalive_tcp_conn_stream_t::keepalive_tcp_conn_stream_t(tcp_conn_t *conn) :
    tcp_conn_stream_t(conn),
    keepalive_callback(NULL) { }

keepalive_tcp_conn_stream_t::~keepalive_tcp_conn_stream_t() {
    // Do nothing
}

void keepalive_tcp_conn_stream_t::set_keepalive_callback(keepalive_callback_t *_keepalive_callback) {
    keepalive_callback = _keepalive_callback;
}

int64_t keepalive_tcp_conn_stream_t::read(void *p, int64_t n) {
    int64_t result = tcp_conn_stream_t::read(p, n);

    if (result > 0 && keepalive_callback != NULL) {
        keepalive_callback->keepalive_read();
    }

    return result;
}

int64_t keepalive_tcp_conn_stream_t::write(const void *p, int64_t n) {
    if (keepalive_callback != NULL) {
        keepalive_callback->keepalive_write();
    }

    return tcp_conn_stream_t::write(p, n);
}

rethread_tcp_conn_stream_t::rethread_tcp_conn_stream_t(tcp_conn_stream_t *conn, threadnum_t thread)
    : conn_(conn), old_thread_(conn->home_thread()), new_thread_(thread) {
    conn->rethread(thread);
    guarantee(conn->home_thread() == thread);
}

rethread_tcp_conn_stream_t::~rethread_tcp_conn_stream_t() {
    guarantee(conn_->home_thread() == new_thread_);
    conn_->rethread(old_thread_);
    guarantee(conn_->home_thread() == old_thread_);
}
