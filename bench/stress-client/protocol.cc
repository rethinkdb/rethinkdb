// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "protocol.hpp"
#include "protocols/memcached_sock_protocol.hpp"
#ifdef USE_LIBMEMCACHED
#  include "protocols/memcached_protocol.hpp"
#endif
#ifdef USE_MYSQL
#  include "protocols/mysql_protocol.hpp"
#endif
#include "protocols/sqlite_protocol.hpp"
#include "protocols/rethinkdb_protocol.hpp"

protocol_t *server_t::connect() {
    switch (protocol) {
    case protocol_rethinkdb:
        return new rethinkdb_protocol_t(host);
    case protocol_sockmemcached:
        return new memcached_sock_protocol_t(host);
#ifdef USE_MYSQL
    case protocol_mysql:
        return new mysql_protocol_t(host);
#endif
#ifdef USE_LIBMEMCACHED
    case protocol_libmemcached:
        return new memcached_protocol_t(host);
#endif
    case protocol_sqlite:
        return new sqlite_protocol_t(host);
    default:
        fprintf(stderr, "Unknown protocol\n");
        exit(-1);
    }
}
