#ifndef CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
#define CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_

#include "containers/archive/archive.hpp"
#include "arch/address.hpp"
#include "arch/types.hpp"


class tcp_conn_stream_t : public read_stream_t, public write_stream_t {
public:
    tcp_conn_stream_t(const char *host, int port, int local_port = 0);
    tcp_conn_stream_t(const ip_address_t &host, int port, int local_port = 0);
    virtual ~tcp_conn_stream_t();

    virtual int64_t read(void *p, int64_t n);
    virtual int64_t write(const void *p, int64_t n);

private:
    tcp_conn_t *conn_;

    DISABLE_COPYING(tcp_conn_stream_t);
};





#endif  // CONTAINERS_ARCHIVE_TCP_CONN_STREAM_HPP_
