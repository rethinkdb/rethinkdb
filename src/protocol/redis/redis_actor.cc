#include "protocol/redis/redis_actor.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/blob.hpp"
#include <iostream>
#include <string>
#include <vector>

enum redis_value_type {
    REDIS_STRING,
    REDIS_LIST,
    REDIS_HASH,
    REDIS_SET,
    REDIS_SORTED_SET
};

//Base class of all redis value types. Contains metadata relevant to all redis value types.
struct redis_value_t {
    uint8_t flags;
    uint32_t metadata_values[0];

    static const uint8_t EXPIRATION_FLAG_MASK = (1 << 4);
    static const uint8_t TYPE_FLAGS_MASK = static_cast<uint8_t>(7 << 5);

    redis_value_type get_redis_type() const {
        //The first three bits encode the redis type
        //for a total of 8 possible types. Redis only
        //currently supports 5 types for now though.
        return static_cast<redis_value_type>(flags >> 5);
    }

    void set_redis_type(redis_value_type type) {
        flags = ((flags | TYPE_FLAGS_MASK) ^ TYPE_FLAGS_MASK) | (type << 5);
    }

    bool expiration_set() const {
        //The 4th bit indicates if the this key has an
        //expiration time set
        return static_cast<bool>(flags & EXPIRATION_FLAG_MASK);
    }

    //Warning, the caller must already have made space for the expiration time value
    void set_expiration(uint32_t expire_time) {
        flags |= EXPIRATION_FLAG_MASK;
        metadata_values[0] = expire_time;
    }

    //Warning, the caller is responsible for unallocating the space thus freed
    bool void_expiration() {
        bool was_set = expiration_set();
        flags = (flags | EXPIRATION_FLAG_MASK) ^ EXPIRATION_FLAG_MASK;
        return was_set;
    }

    size_t get_metadata_size() {
        return sizeof(redis_value_t) + sizeof(uint32_t)*expiration_set();
    }

};

struct redis_string_value_t : redis_value_t {
    size_t get_string_size() const {
        return 0;
    }

    size_t inline_size() const {
        return sizeof(redis_string_value_t) + get_string_size();
    }
};

class redis_value_sizer_t : public value_sizer_t<redis_value_t> {
public:
    redis_value_sizer_t(block_size_t bs) : value_sizer_t<redis_value_t>(bs) {}

    int size(const redis_value_t *value) const {
        size_t size = sizeof(redis_value_t);
        switch(value->get_redis_type()) {
        case REDIS_STRING:
            size = reinterpret_cast<const redis_string_value_t*>(value)->inline_size();
            break;
        case REDIS_LIST:
            break;
        case REDIS_HASH:
            break;
        case REDIS_SET:
            break;
        case REDIS_SORTED_SET:
            break;
        }

        return size;
    }

    virtual bool fits(const redis_value_t *value, int length_available) const {
        int value_size = size(value);
        return value_size <= length_available;
    }

    virtual int max_possible_size() const {
        return sizeof(redis_string_value_t) + 256;
    }

    virtual block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { {'r', 'd', 's', 'l'} };
        return magic;
    }
};

redis_value_sizer_t *sizer;

redis_actor_t::redis_actor_t(btree_slice_t *btree) :
    btree(btree)
{
    sizer = new redis_value_sizer_t(btree->cache()->get_block_size());
}

redis_actor_t::~redis_actor_t() {}

struct unimplemented_result {
    unimplemented_result() { }
    operator status_result () {
        return get_unimplemented_error();
    }

    operator integer_result () {
        return integer_result(get_unimplemented_error());
    }

    operator bulk_result () {
        return bulk_result(get_unimplemented_error());
    }

    operator multi_bulk_result () {
        return multi_bulk_result(get_unimplemented_error());
    }

    status_result get_unimplemented_error() {
        boost::shared_ptr<status_result_struct> result(new status_result_struct);
        result->status = ERROR;
        result->msg = (const char *)("Unimplemented");
        return result;
    }
};


//These macros deliberately match those in the header file. They simply define an "unimplemented
//command" definition for the given command. When adding new commands we can simply copy and paste
//declaration from the header file to get a default implementation. Gradually these will go away
//as we actually define real implementations of the various redis commands.
#define COMMAND_0(RETURN, CNAME) RETURN##_result \
redis_actor_t::CNAME() \
{ \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_1(RETURN, CNAME, ARG_TYPE_ONE) RETURN##_result \
redis_actor_t::CNAME(ARG_TYPE_ONE one) \
{ \
    (void)one; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_2(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO) RETURN##_result \
redis_actor_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two) \
{ \
    (void)one; \
    (void)two; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_3(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE) RETURN##_result \
redis_actor_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two, ARG_TYPE_THREE three) \
{ \
    (void)one; \
    (void)two; \
    (void)three; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_N(RETURN, CNAME) RETURN##_result \
redis_actor_t::CNAME(std::vector<std::string> &one) \
{ \
    (void)one; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
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
/*
integer_result redis_actor_t::append(std::string &key, std::string &toappend) {
    repli_timestamp_t tstamp;
    tstamp.time = 123456;

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_write, 1, tstamp, order_token_t::ignore, &superblock);

    //Construct a btree_key from our key string
    btree_key_buffer_t btree_key(key);

    keyvalue_location_t location;
    find_keyvalue_location_for_write(sizer, &superblock, btree_key.key(), tstamp, &location);

    //This is a string operation so we'll assume that the value is a string value
    if(location.value.get() == NULL) {
        //This key has not already been set, specify its type as a string
        scoped_malloc<value_type_t> smrsv(toappend.size() + sizeof(redis_string_value_t));
        location.value.swap(smrsv);
        redis_string_value_t *value = reinterpret_cast<redis_string_value_t*>(location.value.get());
        value->string_size = 0;
        value->set_redis_type(REDIS_STRING);
    } else {
        redis_string_value_t *value = reinterpret_cast<redis_string_value_t*>(location.value.get());
        if(value->get_redis_type() != REDIS_STRING) {
            //Error, wrong data type
        }
    }

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t*>(location.value.get());
    memcpy(value->string + value->string_size, &toappend.at(0), toappend.size());
    value->string_size += toappend.size();

    apply_keyvalue_change(sizer, &location, btree_key.key(), tstamp);

    return integer_result(value->string_size);
    return integer_result(0);
}
*/

COMMAND_1(integer, decr, string&)
COMMAND_2(integer, decrby, string&, int)

COMMAND_1(bulk, get, string&)
/*
bulk_result redis_actor_t::get(string &key) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    //Construct a btree_key from our key string
    btree_key_buffer_t btree_key(key);

    keyvalue_location_t location;
    find_keyvalue_location_for_read(sizer, &superblock, btree_key.key(), &location);

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t*>(location.value.get());
    static std::string *result = NULL;
    if(value == NULL) {
        //Error key doesn't exist
        std::cout << "Key " << key << " doesn't exist" << std::endl;
    } else {
        if(value->get_redis_type() == REDIS_STRING) {
            result = new std::string(value->string, value->get_string_size());
        } else {
            //Error key isn't a string
            std::cout << "Key " << key << " isn't a string" << std::endl;
        }
    }
    boost::shared_ptr<std::string> res(result);
    return res;
}
*/

COMMAND_2(integer, getbit, string&, unsigned)
COMMAND_3(bulk, getrange, string&, int, int)
COMMAND_2(bulk, getset, string&, string&)
COMMAND_1(integer, incr, string&)
COMMAND_2(integer, incrby, string&, int)
COMMAND_N(multi_bulk, mget)
COMMAND_N(status, mset)
COMMAND_N(integer, msetnx)

status_result redis_error(const char *msg) {
    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = ERROR;
    result->msg = msg;
    return result;
}

//COMMAND_2(status, set, string&, string&)
status_result redis_actor_t::set(string &key, string &val) {
    (void)key;
    (void)val;

    repli_timestamp_t tstamp;
    tstamp.time = 123456;

    //Construct a btree_key from our key string
    btree_key_buffer_t btree_key(key);
    value_txn_t<redis_string_value_t> txn = get_value_write<redis_string_value_t>(btree,
        btree_key.key(), tstamp, order_token_t::ignore);

    //This is a string operation so we'll assume that the value is a string value
    if(txn.value.get() == NULL) {
        //This key has not already been set, allocate space and specify its type as a string
        scoped_malloc<redis_string_value_t> smrsv(sizer->max_possible_size());
        txn.value.swap(smrsv);
    } else {
        //This key has already been set, before we reset it, check if it is a string
        redis_string_value_t *value = txn.value.get();
        if(!value->get_redis_type() != REDIS_STRING) {
            return redis_error("Operation against key holding wrong kind of value");
        }
    }

    redis_string_value_t *value = txn.value.get();
    value->set_redis_type(REDIS_STRING);
    //memcpy(value->string, &val.at(0), value->string_size);

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}


COMMAND_3(integer, setbit, string&, unsigned, unsigned)
COMMAND_3(status, setex, string&, unsigned, string&)
COMMAND_3(integer, setrange, string&, unsigned, string&)
COMMAND_1(integer, Strlen, string&)
