#include "protocol/redis/redis.hpp"
#include <iostream>
#include <string>
#include <vector>

redis_interface_t::redis_interface_t(get_store_t *get_store_p, set_store_interface_t *set_store_p) :
    get_store(get_store_p),
    set_store(set_store_p)
{ }

redis_interface_t::~redis_interface_t() {}

const boost::shared_ptr<status_result> redis_interface_t::set(const std::vector<char> &key, const std::vector<char> &val) {
    (void)key;
    (void)val;
    
    const boost::shared_ptr<status_result> result(new status_result);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}

const boost::shared_ptr<bulk_result> redis_interface_t::get(const std::vector<char> &key) {
    (void)key; 

    const boost::shared_ptr<bulk_result> result(new bulk_result);
    result->push_back('a');
    return result;
}

integer_result redis_interface_t::incr(const std::vector<char> &key) {
    (void)key;

    return 0;
}

integer_result redis_interface_t::decr(const std::vector<char> &key) {
    (void)key;

    return 0;
}

const boost::shared_ptr<multi_bulk_result> redis_interface_t::mget(const std::vector<std::vector<char> > &keys) {
    const boost::shared_ptr<multi_bulk_result> result(new multi_bulk_result());
    result->push_back(keys[0]);
    return result;
}
