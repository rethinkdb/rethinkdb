#include "protocol/memcached2/memcached.hpp"
#include <iostream>
#include <string.h>

memcached_interface_t::memcached_interface_t(tcp_conn_t *conn_p, get_store_t *get_store_p, set_store_interface_t *set_store_p) :
    get_store(get_store_p),
    set_store(set_store_p),
    tcp_conn(conn_p)
{ }

memcached_interface_t::~memcached_interface_t() {}

void memcached_interface_t::do_get(std::vector<char> &key) {
    (void)key;
    /*
    std::string key_str(key.begin(), key.end());
    store_key_t store_key;
    str_to_key(key_str.c_str(), &store_key);
    get_result_t res = get_store->get(store_key, order_token_t());
    if(res.value) {
        //VALUE header
        char buff[1024];
        sprintf(buff, "VALUE %*.*s %u %zu\r\n",
               int(key.size()), int(key.size()), key_str.c_str(), res.flags, res.value->get_size());
        tcp_conn->write(buff, strlen(buff));
        //DATA
        const const_buffer_group_t *bg = res.value->get_data_as_buffers();
        tcp_conn->write(bg->get_buffer(0).data, bg->get_buffer(0).size);
        tcp_conn->write("\r\n", 2);
        tcp_conn->write("END\r\n", 3);
    }
    */
}

void memcached_interface_t::do_set(std::vector<char> &key, unsigned flags, unsigned expiration, std::vector<char> &data) {
    std::cout << "KEY IS: " << std::endl;
    for(unsigned i = 0; i < key.size(); i++) {
        std::cout << key[i] << std::endl;
    }

    std::cout << "DATA IS: " << std::endl;
    for(unsigned i = 0; i < data.size(); i++) {
        std::cout << data[i] << std::endl;
    }

    flags += expiration;
}
