#include "protocol.hpp"
#include "memcached_sock_protocol.hpp"
#ifdef USE_LIBMEMCACHED
#  include "memcached_protocol.hpp"
#endif
#ifdef USE_MYSQL
#  include "mysql_protocol.hpp"
#endif
#include "sqlite_protocol.hpp"

protocol_t *server_t::connect(load_t *load) {
    switch (protocol) {
    case protocol_sockmemcached:
        return new memcached_sock_protocol_t(host, load);
#ifdef USE_MYSQL
    case protocol_mysql:
        return new mysql_protocol_t(host, load);
#endif
#ifdef USE_LIBMEMCACHED
    case protocol_libmemcached:
        return new memcached_protocol_t(host, load);
#endif
    case protocol_sqlite:
        return new sqlite_protocol_t(host, load);
    default:
        fprintf(stderr, "Unknown protocol\n");
        exit(-1);
    }
}
