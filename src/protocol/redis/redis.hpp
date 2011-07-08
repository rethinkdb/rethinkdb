#ifndef __PROTOCOL_REDIS_REDIS_HPP__
#define __PROTOCOL_REDIS_REDIS_HPP__

#include "arch/arch.hpp"
#include "store.hpp"
#include <vector>
#include <boost/shared_ptr.hpp>

enum redis_status {
    OK,
    ERROR
};

struct status_result {
    redis_status status;
    const char *msg;
};

typedef unsigned integer_result;
typedef std::vector<char> bulk_result;
typedef std::vector<std::vector<char> > multi_bulk_result;

struct redis_interface_t {
    redis_interface_t(get_store_t *get_store, set_store_interface_t *set_store);
    ~redis_interface_t();

    const boost::shared_ptr<status_result> set(const std::vector<char> &key, const std::vector<char> &val);
    const boost::shared_ptr<bulk_result> get(const std::vector<char> &key);
    integer_result incr(const std::vector<char> &key);
    integer_result decr(const std::vector<char> &key);
    const boost::shared_ptr<multi_bulk_result> mget(const std::vector<std::vector<char> > &keys);

protected:
    get_store_t *get_store;
    set_store_interface_t *set_store;
};

#endif /* __PROTOCOL_REDIS_REDIS_HPP__ */
