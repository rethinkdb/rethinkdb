#include "protocol.hpp"
#include "memcached_sock_protocol.hpp"
#ifdef USE_LIBMEMCACHED
#  include "memcached_protocol.hpp"
#endif
#ifdef USE_MYSQL
#  include "mysql_protocol.hpp"
#endif
#include "sqlite_protocol.hpp"

protocol_t *server_t::connect() {
    switch (protocol) {
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
