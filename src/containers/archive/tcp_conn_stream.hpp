// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
#define CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_

#include "arch/address.hpp"
#include "arch/io/openssl.hpp"
#include "arch/types.hpp"
#include "containers/archive/archive.hpp"
#include "threading.hpp"

class signal_t;

class tcp_conn_stream_t : public read_stream_t, public write_stream_t {
public:
    tcp_conn_stream_t(
        tls_ctx_t *tls_ctx, const ip_address_t &host, int port,
        signal_t *interruptor, int local_port = 0);

    // Takes ownership.
    explicit tcp_conn_stream_t(tcp_conn_t *conn);
    virtual ~tcp_conn_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);
    virtual MUST_USE int64_t write(const void *p, int64_t n);
    virtual MUST_USE int64_t write_buffered(const void *p, int64_t n);
    virtual bool flush_buffer();

    void rethread(threadnum_t new_thread);

    threadnum_t home_thread() const;

    void shutdown_read();
    void shutdown_write();
    bool is_read_open();
    bool is_write_open();

    tcp_conn_t *get_underlying_conn() {
        return conn_;
    }

private:
    tcp_conn_t *conn_;

    DISABLE_COPYING(tcp_conn_stream_t);
};

// Wraps around a `tcp_conn_stream_t` and redirects `write()` to `write_buffered()`
class make_buffered_tcp_conn_stream_wrapper_t : public write_stream_t {
public:
    explicit make_buffered_tcp_conn_stream_wrapper_t(tcp_conn_stream_t *inner);
    virtual MUST_USE int64_t write(const void *p, int64_t n);
private:
    tcp_conn_stream_t *inner_;
};

class keepalive_tcp_conn_stream_t : public tcp_conn_stream_t {
public:
    keepalive_tcp_conn_stream_t(
        tls_ctx_t *tls_ctx, const ip_address_t &host, int port,
        signal_t *interruptor, int local_port = 0);

    // Takes ownership.
    explicit keepalive_tcp_conn_stream_t(tcp_conn_t *conn);
    virtual ~keepalive_tcp_conn_stream_t();

    class keepalive_callback_t {
    public:
        virtual ~keepalive_callback_t() { }
        virtual void keepalive_read() = 0;
        virtual void keepalive_write() = 0;
    };

    void set_keepalive_callback(keepalive_callback_t *_keepalive_callback);

    virtual MUST_USE int64_t read(void *p, int64_t n);
    virtual MUST_USE int64_t write(const void *p, int64_t n);
    virtual MUST_USE int64_t write_buffered(const void *p, int64_t n);
    virtual bool flush_buffer();

private:
    keepalive_callback_t *keepalive_callback;
};


class rethread_tcp_conn_stream_t {
public:
    rethread_tcp_conn_stream_t(tcp_conn_stream_t *conn, threadnum_t thread);
    ~rethread_tcp_conn_stream_t();
private:
    tcp_conn_stream_t *conn_;
    threadnum_t old_thread_;
    threadnum_t new_thread_;
    DISABLE_COPYING(rethread_tcp_conn_stream_t);
};




#endif  // CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
