// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
#define CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_

#include "containers/archive/archive.hpp"
#include "arch/address.hpp"
#include "arch/types.hpp"

class signal_t;

class tcp_conn_stream_t : public read_stream_t, public write_stream_t {
public:
    tcp_conn_stream_t(const ip_address_t &host, int port, signal_t *interruptor, int local_port = 0);

    // Takes ownership.
    explicit tcp_conn_stream_t(tcp_conn_t *conn);
    virtual ~tcp_conn_stream_t();

    virtual MUST_USE int64_t read(void *p, int64_t n);
    virtual MUST_USE int64_t write(const void *p, int64_t n);

    void rethread(int new_thread);

    int home_thread() const;

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

class rethread_tcp_conn_stream_t {
public:
    rethread_tcp_conn_stream_t(tcp_conn_stream_t *conn, int thread);
    ~rethread_tcp_conn_stream_t();
private:
    tcp_conn_stream_t *conn_;
    int old_thread_, new_thread_;
    DISABLE_COPYING(rethread_tcp_conn_stream_t);
};




#endif  // CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
