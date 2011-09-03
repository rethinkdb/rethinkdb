#include "protocol/redis/redis_types.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/slice.hpp"
#include <iostream>
#include <string>
#include <vector>

struct keys_to_region : boost::static_visitor<redis_protocol_t::region_t> {
    redis_protocol_t::region_t operator()(std::vector<std::string> &keys) const {
        //TODO fix this 
        store_key_t s_key(keys[0]);
        redis_protocol_t::region_t r(key_range_t::open, s_key, key_range_t::closed, s_key);
        return r;
    }

    redis_protocol_t::region_t operator()(std::string key) const {
        store_key_t s_key(key);
        redis_protocol_t::region_t r(key_range_t::open, s_key, key_range_t::closed, s_key);
        return r;
    }
};

redis_protocol_t::region_t redis_protocol_t::read_operation_t::get_region() {
    boost::variant<std::vector<std::string>, std::string> keys = get_keys();
    return boost::apply_visitor(keys_to_region(), keys);
}

redis_protocol_t::region_t redis_protocol_t::write_operation_t::get_region() {
    boost::variant<std::vector<std::string>, std::string> keys = get_keys();
    return boost::apply_visitor(keys_to_region(), keys);
}

redis_protocol_t::read_response_t redis_protocol_t::read_response_t::unshard(
        std::vector<redis_protocol_t::read_response_t> &responses) {
    read_response_t result(responses[0]);
    for(std::vector<read_response_t>::iterator iter = responses.begin() + 1; iter != responses.end(); ++iter) {
        result->deshard(iter->get());
    }

    return result;
}

redis_protocol_t::read_response_t redis_protocol_t::read_response_t::unparallelize(
        std::vector<redis_protocol_t::read_response_t> &responses) {
    read_response_t result(responses[0]);
    for(std::vector<read_response_t>::iterator iter = responses.begin() + 1; iter != responses.end(); ++iter) {
        result->deparallelize(iter->get());
    }

    return result;
}

redis_protocol_t::write_response_t
        redis_protocol_t::write_response_t::unshard(std::vector<write_response_t> &responses) {
    write_response_t result(responses[0]);
    for(std::vector<write_response_t>::iterator iter = responses.begin() + 1; iter != responses.end(); ++iter) {
        result->deshard(iter->get());
    }

    return result;
}


// redis_protocol_t::store_t

void redis_protocol_t::store_t::create(serializer_t *ser, redis_protocol_t::region_t region) {
    (void)region;

    mirrored_cache_static_config_t cache_static_config;
    cache_t::create(ser, &cache_static_config);
    mirrored_cache_config_t cache_dynamic_config;
    cache_t cache(ser, &cache_dynamic_config);
    btree_slice_t::create(&cache);
}

redis_protocol_t::store_t::store_t(serializer_t *ser, redis_protocol_t::region_t region_) :
    cache(ser, &cache_config),
    btree(&cache),
    region(region_)
    { }

redis_protocol_t::store_t::~store_t() {

}

redis_protocol_t::region_t redis_protocol_t::store_t::get_region() {
    return region;
}

redis_protocol_t::read_response_t redis_protocol_t::store_t::read(redis_protocol_t::read_t read, order_token_t otok) {
    try {
        read_response_t response = read->execute(&btree, otok);
        return response;
    } catch(const char *msg) {
        return read_response_t(new error_result_t(msg));
    }
}

redis_protocol_t::write_response_t redis_protocol_t::store_t::write(redis_protocol_t::write_t write, redis_protocol_t::timestamp_t timestamp, order_token_t otok) {
    try {
        write_response_t response = write->execute(&btree, timestamp, otok);
        return response;
    } catch(const char *msg) {
        return write_response_t(new error_result_t(msg));
    }
}


// Individual commands are implemented in their respective files: keys, strings, etc.
