#include "protocol/redis/redis_util.hpp"
#include <boost/lexical_cast.hpp>
#include <iostream>

redis_actor_t::redis_actor_t(btree_slice_t *btree) :
    btree(btree)
{ }

redis_actor_t::~redis_actor_t() {}

//KEYS

//COMMAND_N(integer, del)
integer_result redis_actor_t::del(std::vector<std::string> &keys) {
    repli_timestamp_t tstamp;
    tstamp.time = 1234;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());

    int count = 0;
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {

        got_superblock_t superblock;
        get_btree_superblock(btree, rwi_write, 1, tstamp, order_token_t::ignore, &superblock);

        btree_key_buffer_t btree_key(*iter);
        keyvalue_location_t<redis_value_t> location;
        find_keyvalue_location_for_write(&sizer, &superblock, btree_key.key(), tstamp, &location);

        if(location.value.get()) {
            // Deallocate the value for this type
            switch(location.value->get_redis_type()) {
            case REDIS_STRING:
                reinterpret_cast<redis_string_value_t *>(location.value.get())->clear(location.txn.get());
                break;
            // TODO other types
            case REDIS_LIST:
                break;
            case REDIS_HASH:
                break;
            case REDIS_SET:
                break;
            case REDIS_SORTED_SET:
                break;
            }

            scoped_malloc<redis_value_t> null;
            location.value.swap(null);
            count++;

            apply_keyvalue_change(&sizer, &location, btree_key.key(), tstamp);
        }
    }

    return integer_result(count);
}

//COMMAND_1(integer, exists, string&)
integer_result redis_actor_t::exists(std::string &key) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    if(location.value.get()) {
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
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_value_t *value = location.value.get();
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
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_value_t *value = location.value.get();

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

// Not a redis interface function, used to implement all the 'crement operations
integer_result redis_actor_t::crement(std::string &key, int by) {
    set_oper_t oper(key, btree, 12345);
    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());

    if( value == NULL
     || value->get_redis_type() != REDIS_STRING
      ) {
        return integer_result(redis_error("value is not an integer or out of range"));
    }

    // Get the string out and into a string
    blob_t blob(value->get_content(), blob::btree_maxreflen);

    std::string int_string;
    blob.read_to_string(int_string, oper.location.txn.get(), 0, blob.valuesize());

    // We'll be writing in the new value from scratch
    blob.clear(oper.location.txn.get());

    // Convert string to integer
    int64_t int_val;
    try {
        int_val = boost::lexical_cast<int64_t>(int_string);
    } catch(boost::bad_lexical_cast &) {
        return integer_result(redis_error("value is not an integer or out of range"));
    }
    int_val += by;
    int_string = boost::lexical_cast<std::string>(int_val);

    blob.append_region(oper.location.txn.get(), int_string.size());
    blob.write_from_string(int_string, oper.location.txn.get(), 0);
    
    return integer_result(int_val);
}

// Not part of redis interface, used to implement various get funtions
std::string *redis_actor_t::get_string(string &key) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    //Construct a btree_key from our key string
    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(location.value.get());
    std::string *result = NULL;
    if(value == NULL) {
        //Error key doesn't exist
    } else if(value->get_redis_type() != REDIS_STRING) {
        //Error key isn't a string
        //return redis_error("Operation against key holding the wrong value");
    } else {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        int64_t val_size = blob.valuesize();
        result = new std::string(val_size, '\0');

        buffer_group_t dest;
        dest.add_buffer(val_size, const_cast<char *>(result->c_str()));

        buffer_group_t src;
        boost::scoped_ptr<blob_acq_t> acq(new blob_acq_t);
        blob.expose_region(location.txn.get(), rwi_read, 0, val_size, &src, acq.get());
        buffer_group_copy_data(&dest, const_view(&src));
    }

    return result;
}
