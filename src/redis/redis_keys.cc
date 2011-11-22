#ifndef NO_REDIS
#include "redis/redis_util.hpp"
#include "redis/redis.hpp"

// KEYS commands

//WRITE(del)
KEYS(del)
SHARD_W(del)

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
    if(oper.location.value.get() == NULL) {
        // Key not found, return 0
        return int_response(0);
    }

    oper.expire_at(two);
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

//READ(rename_get_type)
KEYS(rename_get_type)
SHARD_R(rename_get_type)

EXECUTE_R(rename_get_type) {
    read_oper_t oper(one, btree, otok);
    redis_value_t *value = oper.location.value.get();
    if(!value) {
        return read_response_t();
    } else {
        return int_response(value->get_redis_type());
    }
}

//READ(ttl)
KEYS(ttl)
SHARD_R(ttl)

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

//READ(type)
KEYS(type)
SHARD_R(type)

EXECUTE_R(type) {
    read_oper_t oper(one, btree, otok);
    redis_value_t *value = oper.location.value.get();

    ok_result_t *result = NULL;

    if(!value) {
        result = new ok_result_t("none");
    } else {
        switch(value->get_redis_type()) {
        case REDIS_STRING:
            result = new ok_result_t("string");
            break;
        case REDIS_LIST:
            result = new ok_result_t("list");
            break;
        case REDIS_HASH:
            result = new ok_result_t("hash");
            break;
        case REDIS_SET:
            result = new ok_result_t("set");
            break;
        case REDIS_SORTED_SET:
            result = new ok_result_t("zset");
            break;
        default:
            unreachable();
            break;
        }
    }
    
    return read_response_t(result);
}
#endif //#ifndef NO_REDIS
