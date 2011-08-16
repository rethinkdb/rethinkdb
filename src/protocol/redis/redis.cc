#include "protocol/redis/redis_types.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/slice.hpp"
#include <iostream>
#include <string>
#include <vector>

struct keys_to_region : boost::static_visitor<redis_protocol_t::region_t> {
    redis_protocol_t::region_t operator()(std::vector<std::string> &keys) const {
        (void) keys;
        // TODO implement this based on region
        return redis_protocol_t::region_t();
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
    btree(&cache, 1000),
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

/*

//KEYS
COMMAND_N(integer, del)
COMMAND_1(integer, exists, string&)
COMMAND_2(integer, expire, string&, unsigned)
COMMAND_2(integer, expireat, string&, unsigned)
COMMAND_1(multi_bulk, keys, string&)
COMMAND_2(integer, move, string&, string&)
COMMAND_1(integer, persist, string&)
COMMAND_0(bulk, randomkey)
COMMAND_2(status, rename, string&, string&)
COMMAND_2(integer, renamenx, string&, string&)
COMMAND_1(integer, ttl, string&)
COMMAND_1(status, type, string&)

//Strings
COMMAND_2(integer, append, string&, string&)
COMMAND_1(integer, decr, string&)
COMMAND_2(integer, decrby, string&, int)
COMMAND_1(bulk, get, string&)
COMMAND_2(integer, getbit, string&, unsigned)
COMMAND_3(bulk, getrange, string&, int, int)
COMMAND_2(bulk, getset, string&, string&)
COMMAND_1(integer, incr, string&)
COMMAND_2(integer, incrby, string&, int)
COMMAND_N(multi_bulk, mget)
COMMAND_N(status, mset)
COMMAND_N(integer, msetnx)
COMMAND_2(status, set, string&, string&)
COMMAND_3(integer, setbit, string&, unsigned, unsigned)
COMMAND_3(status, setex, string&, unsigned, string&)
COMMAND_3(integer, setrange, string&, unsigned, string&)
COMMAND_1(integer, Strlen, string&)

//Hashes
COMMAND_N(integer, hdel)
COMMAND_2(integer, hexists, string&, string&)
COMMAND_2(bulk, hget, string&, string&)
COMMAND_1(multi_bulk, hgetall, string&)
COMMAND_3(integer, hincrby, string&, string&, int)
COMMAND_1(multi_bulk, hkeys, string&)
COMMAND_1(integer, hlen, string&)
COMMAND_N(multi_bulk, hmget)
COMMAND_N(status, hmset)
COMMAND_3(integer, hset, string&, string&, string&)
COMMAND_3(integer, hsetnx, string&, string&, string&)
COMMAND_1(multi_bulk, hvals, string&)

//Sets
COMMAND_N(integer, sadd)
COMMAND_1(integer, scard, string&)
COMMAND_N(multi_bulk, sdiff)
COMMAND_N(integer, sdiffstore)
COMMAND_N(multi_bulk, sinter)
COMMAND_N(integer, sinterstore)
COMMAND_2(integer, sismember, string&, string&)
COMMAND_1(multi_bulk, smembers, string&)
COMMAND_3(integer, smove, string&, string&, string&)
COMMAND_1(bulk, spop, string&)
COMMAND_1(bulk, srandmember, string&)
COMMAND_N(integer, srem)
COMMAND_N(multi_bulk, sunion)
COMMAND_N(integer, sunionstore)

*/
