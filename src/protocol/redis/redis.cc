#include "protocol/redis/redis_types.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/slice.hpp"
#include "import/import.hpp"
#include <iostream>
#include <string>
#include <vector>

#include "serializer/log/log_serializer.hpp"

redis_interface_t::redis_interface_t() {
    //DUMMY stack
    cmd_config_t config;
    config.store_dynamic_config.cache.max_dirty_size = config.store_dynamic_config.cache.max_size / 10;
    char filename[200];
    snprintf(filename, sizeof(filename), "rethinkdb_data_%d", 0);
    log_serializer_private_dynamic_config_t ser_config;
    ser_config.db_filename = filename;

    log_serializer_t::create(&config.store_dynamic_config.serializer, &ser_config, &config.store_static_config.serializer);
    log_serializer_t *serializer = new log_serializer_t(&config.store_dynamic_config.serializer, &ser_config);

    std::vector<serializer_t *> *serializers = new std::vector<serializer_t *>();
    serializers->push_back(serializer);
    serializer_multiplexer_t::create(*serializers, 1);
    serializer_multiplexer_t *multiplexer = new serializer_multiplexer_t(*serializers);

    cache_t::create(multiplexer->proxies[0], &config.store_static_config.cache);
    cache_t *cache = new cache_t(multiplexer->proxies[0], &config.store_dynamic_config.cache);

    btree_slice_t::create(cache);
    slice = new btree_slice_t(cache, 1000);

    actor = new redis_actor_t(slice);
}

redis_interface_t::~redis_interface_t() {}

#define COMMAND_0(RETURN, CNAME) RETURN##_result \
redis_interface_t::CNAME() \
{ \
    on_thread_t thread_switcher(slice->home_thread()); \
    return actor->CNAME(); \
}

#define COMMAND_1(RETURN, CNAME, ARG_TYPE_ONE) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one) \
{ \
    on_thread_t thread_switcher(slice->home_thread()); \
    return actor->CNAME(one); \
}

#define COMMAND_2(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two) \
{ \
    on_thread_t thread_switcher(slice->home_thread()); \
    return actor->CNAME(one, two); \
}

#define COMMAND_3(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two, ARG_TYPE_THREE three) \
{ \
    on_thread_t thread_switcher(slice->home_thread()); \
    return actor->CNAME(one, two, three); \
}

#define COMMAND_N(RETURN, CNAME) RETURN##_result \
redis_interface_t::CNAME(std::vector<std::string> &one) \
{ \
    on_thread_t thread_switcher(slice->home_thread()); \
    return actor->CNAME(one); \
}

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
