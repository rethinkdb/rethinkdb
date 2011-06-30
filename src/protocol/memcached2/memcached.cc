#include "memcached.hpp"
#include <iostream>

memcached_interface_t::memcached_interface_t(tcp_conn_t *conn_p, get_store_t *get_store_p, set_store_interface_t *set_store_p) :
    get_store(get_store_p),
    set_store(set_store_p),
    tcp_conn(conn_p)
{ }

memcached_interface_t::~memcached_interface_t() {}

void memcached_interface_t::do_get(std::string &key) {
    std::cout << "KEY IS: " << key << std::endl;
    /*
    for(unsigned i = 0; i < key.size(); i++) {
        std::cout << key[i] << std::endl;
    }
    */
}

void memcached_interface_t::do_set(std::string &key, unsigned flags, unsigned expiration, std::vector<char> &data) {
    std::cout << "KEY IS: " << key << std::endl;
    /*
    for(unsigned i = 0; i < key.size(); i++) {
        std::cout << key[i] << std::endl;
    }
    */

    std::cout << "DATA IS: " << std::endl;
    for(unsigned i = 0; i < data.size(); i++) {
        std::cout << data[i] << std::endl;
    }

    flags += expiration;
}
