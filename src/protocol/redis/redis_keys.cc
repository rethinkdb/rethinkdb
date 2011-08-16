#include "protocol/redis/redis_util.hpp"
#include "protocol/redis/redis.hpp"

// KEYS commands

//WRITE(del)
KEYS(del)

std::vector<redis_protocol_t::write_t>
        redis_protocol_t::del::shard(std::vector<redis_protocol_t::region_t> &regions) {
    (void)regions;
    std::vector<boost::shared_ptr<redis_protocol_t::write_operation_t> > result;
    for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end(); ++iter) {
        std::vector<std::string> keys;
        keys.push_back(*iter);
        boost::shared_ptr<redis_protocol_t::write_operation_t> d(new del(keys));
        result.push_back(d);
    }

    return result;
}

EXECUTE_W(del) {
    int count = 0;
    for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end(); ++iter) {
        set_oper_t oper(*iter, btree, timestamp, otok);
        if(oper.del()) count++;
    }

    return write_response_t(new sum_integer_t(count));
}

//READ(exists)
KEYS(exists)
SHARD_R(exists)
PARALLEL(exists)

EXECUTE_R(exists) {
    read_oper_t oper(one, btree, otok);

    if(oper.exists()) {
        return int_response(1);
    } else {
        return int_response(0);
    }
}

//WRITE(expire)
KEYS(expire)
SHARD_W(expire)

EXECUTE_W(expire) {
    // TODO real expire time from the timestamp
    uint32_t expire_time = time(NULL) + two;
    expireat expr(one, expire_time);
    return expr.execute(btree, timestamp, otok);
}

//WRITE(expireat)
KEYS(expireat)
SHARD_W(expireat)

EXECUTE_W(expireat) {
    set_oper_t oper(one, btree, timestamp, otok);
    redis_value_t *value = oper.location.value.get();
    if(value == NULL) {
        // Key not found, return 0
        return int_response(0);
    }

    if(!value->expiration_set()) {
        // We must clear space for the new metadata first
        value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
        int data_size = sizer.size(value) - value->get_metadata_size();
        memmove(value->get_content() + sizeof(uint32_t), value->get_content(), data_size);
    }

    oper.location.value->set_expiration(two);

    return int_response(1);
}

READ(keys)
WRITE(move)

//WRITE(persist)
KEYS(persist)
SHARD_W(persist)

EXECUTE_W(persist) {
    set_oper_t oper(one, btree, timestamp, otok);
    redis_value_t *value = oper.location.value.get();
    if(value == NULL || !value->expiration_set()) {
        return int_response(0);
    }

    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    int data_size = sizer.size(value) - value->get_metadata_size();

    value->void_expiration();
    memmove(value->get_content(), value->get_content() + sizeof(uint32_t), data_size);

    return int_response(1);
}

READ(randomkey)
WRITE(rename)
WRITE(renamenx)

//READ(ttl)
KEYS(ttl)
SHARD_R(ttl)
PARALLEL(ttl)

EXECUTE_R(ttl) {
    read_oper_t oper(one, btree, otok);

    redis_value_t *value = oper.location.value.get();
    if(value == NULL || !value->expiration_set()) {
        return int_response(-1);
    }

    // TODO get correct current time
    int ttl = value->get_expiration() - time(NULL);
    if(ttl < 0) {
        // Then this key has technically expired
        // TODO figure out a way to check and delete expired keys
        return int_response(-1);
    }
    
    return int_response(ttl);
}

READ(type)

/*
COMMAND_1(multi_bulk, keys, string&)
COMMAND_2(integer, move, string&, string&)

COMMAND_0(bulk, randomkey)
COMMAND_2(status, rename, string&, string&)
COMMAND_2(integer, renamenx, string&, string&)

//COMMAND_1(integer, ttl, string&)


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
*/
