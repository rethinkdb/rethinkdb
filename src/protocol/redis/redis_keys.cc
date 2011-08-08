#include "protocol/redis/redis_util.hpp"
#include <boost/lexical_cast.hpp>
#include <iostream>

// These need to be implemented somewhere...
redis_actor_t::redis_actor_t(btree_slice_t *btree) :
    btree(btree)
{ }

redis_actor_t::~redis_actor_t() {}

// KEYS commands

//COMMAND_N(integer, del)
integer_result redis_actor_t::del(std::vector<std::string> &keys) {
    int count = 0;
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        set_oper_t oper(*iter, btree, 1234);
        if(oper.del()) count++;
    }

    return integer_result(count);
}

//COMMAND_1(integer, exists, string&)
integer_result redis_actor_t::exists(std::string &key) {
    read_oper_t oper(key, btree);

    if(oper.exists()) {
        return integer_result(1);
    } else {
        return integer_result(0);
    }
}

//COMMAND_2(integer, expire, string&, unsigned)
integer_result redis_actor_t::expire(std::string &key, unsigned timeout) {
    // TODO real expire time from the timestamp
    uint32_t expire_time = time(NULL) + timeout;
    return expireat(key, expire_time);
}

//COMMAND_2(integer, expireat, string&, unsigned)
integer_result redis_actor_t::expireat(std::string &key, unsigned time) {
    set_oper_t oper(key, btree, 1234);
    redis_value_t *value = oper.location.value.get();
    if(value == NULL) {
        // Key not found, return 0
        return integer_result(0);
    }

    if(!value->expiration_set()) {
        // We must clear space for the new metadata first
        value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
        int data_size = sizer.size(value) - value->get_metadata_size();
        memmove(value->get_content() + sizeof(uint32_t), value->get_content(), data_size);
    }

    oper.location.value->set_expiration(time);

    return integer_result(1);
}

COMMAND_1(multi_bulk, keys, string&)
COMMAND_2(integer, move, string&, string&)

//COMMAND_1(integer, persist, string&)
integer_result redis_actor_t::persist(std::string &key) {
    set_oper_t oper(key, btree, 1234);
    redis_value_t *value = oper.location.value.get();
    if(value == NULL || !value->expiration_set()) {
        return integer_result(0);
    }

    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    int data_size = sizer.size(value) - value->get_metadata_size();

    value->void_expiration();
    memmove(value->get_content(), value->get_content() + sizeof(uint32_t), data_size);

    return integer_result(1);
}

COMMAND_0(bulk, randomkey)
COMMAND_2(status, rename, string&, string&)
COMMAND_2(integer, renamenx, string&, string&)

//COMMAND_1(integer, ttl, string&)
integer_result redis_actor_t::ttl(std::string &key) {
    read_oper_t oper(key, btree);

    redis_value_t *value = oper.location.value.get();
    if(value == NULL || !value->expiration_set()) {
        return integer_result(-1);
    }

    // TODO get correct current time
    int ttl = value->get_expiration() - time(NULL);
    if(ttl < 0) {
        // Then this key has technically expired
        // TODO figure out a way to check and delete expired keys
        return integer_result(-1);
    }
    
    return integer_result(ttl);
}

//COMMAND_1(status, type, string&)
status_result redis_actor_t::type(std::string &key) {
    read_oper_t oper(key, btree);
    
    redis_value_t *value = oper.location.value.get();

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    switch(value->get_redis_type()) {
    case REDIS_STRING:
        result->msg = (const char *)("string");
        break;
    case REDIS_LIST:
        result->msg = (const char *)("list");
        break;
    case REDIS_HASH:
        result->msg = (const char *)("hash");
        break;
    case REDIS_SET:
        result->msg = (const char *)("set");
        break;
    case REDIS_SORTED_SET:
        result->msg = (const char *)("zset");
        break;
    default:
        assert(0);
        break;
    }
    
    return result;
}
