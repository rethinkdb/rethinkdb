#ifndef __MEMCACHED_MEMCACHED_HPP__
#define __MEMCACHED_MEMCACHED_HPP__

#include "arch/arch.hpp"
#include "store.hpp"
#include <vector>

struct memcached_interface_t {
    memcached_interface_t(tcp_conn_t *conn, get_store_t *get_store, set_store_interface_t *set_store);
    ~memcached_interface_t();

    void do_get(std::string &key);
    void do_set(std::string &key, unsigned flags, unsigned expiration, std::vector<char> &data);
 
private:
    get_store_t *get_store;
    set_store_interface_t *set_store;
    //TODO generic file or tcp conn, potentially using iterators
    tcp_conn_t *tcp_conn;
};

#endif /* __MEMCACHED_MEMCACHED_HPP__ */
