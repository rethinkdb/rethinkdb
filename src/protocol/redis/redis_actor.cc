#include "protocol/redis/redis_actor.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/buffer_group.hpp"
#include "protocol/redis/redis_types.hpp"
#include "btree/iteration.hpp"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <vector>

redis_actor_t::redis_actor_t(btree_slice_t *btree) :
    btree(btree)
{ }

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

status_result redis_error(const char *msg) {
    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = ERROR;
    result->msg = msg;
    return result;
}

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

// Not part of redis interface, used to implement set operations
struct set_oper_t {
    set_oper_t(std::string &key, btree_slice_t *btree, int64_t timestamp) :
        sizer(btree->cache()->get_block_size()),
        btree_key(key)
    {
        tstamp.time = timestamp;

        // Get the superblock that represents our write transaction
        get_btree_superblock(btree, rwi_write, 1, tstamp, order_token_t::ignore, &superblock);
        find_keyvalue_location_for_write(&sizer, &superblock, btree_key.key(), tstamp, &location);
    }

    ~set_oper_t() {
        apply_keyvalue_change(&sizer, &location, btree_key.key(), tstamp);
    }

    keyvalue_location_t<redis_value_t> location;
    repli_timestamp_t tstamp;

protected:
    got_superblock_t superblock;
    value_sizer_t<redis_value_t> sizer;
    btree_key_buffer_t btree_key;
};

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

//Strings
//COMMAND_2(integer, append, string&, string&)
integer_result redis_actor_t::append(std::string &key, std::string &toappend) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }
    
    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    
    uint64_t old_size = blob.valuesize();
    blob.append_region(oper.location.txn.get(), toappend.size());
    blob.write_from_string(toappend, oper.location.txn.get(), old_size);

    return integer_result(blob.valuesize());
}

//COMMAND_1(integer, decr, string&)
integer_result redis_actor_t::decr(std::string &key) {
    return crement(key, -1);
}

//COMMAND_2(integer, decrby, string&, int)
integer_result redis_actor_t::decrby(std::string &key, int by) {
    return crement(key, -by);
}

//COMMAND_1(bulk, get, string&)
bulk_result redis_actor_t::get(string &key) {
    std::string *result = get_string(key);
    boost::shared_ptr<std::string> res(result);
    return bulk_result(res);
}

//COMMAND_2(integer, getbit, string&, unsigned)
integer_result redis_actor_t::getbit(string &key, unsigned bit) {
    std::string *full_string = get_string(key);
    char byte = full_string->at(bit / 8);
    // TODO check that this is the right bit ordering
    return !!(byte & (1 << (bit % 8)));
}

//COMMAND_3(bulk, getrange, string&, int, int)
bulk_result redis_actor_t::getrange(string &key, int start, int end) {
    std::string *full_string = get_string(key);
    std::string *result = new std::string(full_string->begin() + start, full_string->begin() + end);
    boost::shared_ptr<std::string> res(result);
    return bulk_result(res);
}

//COMMAND_2(bulk, getset, string&, string&)
bulk_result redis_actor_t::getset(std::string &key, std::string &new_val) {
    set_oper_t oper(key, btree, 1234);

    std::string *result = NULL;
    redis_string_value_t *value; 
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
        value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return redis_error("Operation against key holding wrong kind of value");
    } else {
        value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
        result = new std::string();
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(*result, oper.location.txn.get(), 0, blob.valuesize());
        blob.clear(oper.location.txn.get());
    }

    blob_t blob(value->get_content(), blob::btree_maxreflen);
    blob.append_region(oper.location.txn.get(), new_val.size());
    blob.write_from_string(new_val, oper.location.txn.get(), 0);

    return bulk_result(boost::shared_ptr<std::string>(result));
}

//COMMAND_1(integer, incr, string&)
integer_result redis_actor_t::incr(std::string &key) {
    return crement(key, 1);
}

//COMMAND_2(integer, incrby, string&, int)
integer_result redis_actor_t::incrby(std::string &key, int by) {
    return crement(key, by);
}

//COMMAND_N(multi_bulk, mget)
multi_bulk_result redis_actor_t::mget(std::vector<std::string> &keys) {
    boost::shared_ptr<std::vector<std::string> > result =
        boost::shared_ptr<std::vector<std::string> >(new std::vector<std::string>());
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        result->push_back(*get_string(*iter));
    }
    return multi_bulk_result(result);
}

//COMMAND_N(status, mset)
status_result redis_actor_t::mset(std::vector<std::string> &keys_vals) {
    for(std::vector<std::string>::iterator iter = keys_vals.begin(); iter != keys_vals.end(); iter += 2) {
        set(*iter, *(iter + 1));
    }

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}

COMMAND_N(integer, msetnx)

//COMMAND_2(status, set, string&, string&)
status_result redis_actor_t::set(string &key, string &val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return redis_error("Operation against key holding wrong kind of value");
    }

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    blob.clear(oper.location.txn.get());

    blob.append_region(oper.location.txn.get(), val.size());
    blob.write_from_string(val, oper.location.txn.get(), 0);

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = (const char *)("OK");
    return result;
}

//COMMAND_3(integer, setbit, string&, unsigned, unsigned)
integer_result redis_actor_t::setbit(std::string &key, unsigned bit_index, unsigned bit_val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }

    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    int64_t difference = (bit_index / 8 + 1) - blob.valuesize();
    if(difference > 0) {
        blob.append_region(oper.location.txn.get(), difference);
    }

    buffer_group_t dest;
    boost::scoped_ptr<blob_acq_t> acq(new blob_acq_t);
    blob.expose_region(oper.location.txn.get(), rwi_write, bit_index / 8, 1, &dest, acq.get());

    buffer_group_t::buffer_t buff =  dest.get_buffer(0);
    int old_val = !!(reinterpret_cast<uint8_t *>(buff.data)[0] & (1 << (bit_index % 8)));
    reinterpret_cast<uint8_t *>(buff.data)[0] =
        (reinterpret_cast<uint8_t *>(buff.data)[0] | (1 << (bit_index % 8))) ^ (!!!bit_val << (bit_index % 8));

    return integer_result(old_val);
}

COMMAND_3(status, setex, string&, unsigned, string&)

//COMMAND_3(integer, setrange, string&, unsigned, string&)
integer_result redis_actor_t::setrange(std::string &key, unsigned offset, std::string &val) {
    set_oper_t oper(key, btree, 1234);
    if(oper.location.value.get() == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        oper.location.value->set_redis_type(REDIS_STRING);
    } else if(oper.location.value->get_redis_type() != REDIS_STRING) {
        return integer_result(redis_error("Operation against key holding wrong kind of value"));
    }
     
    redis_string_value_t *value = reinterpret_cast<redis_string_value_t *>(oper.location.value.get());
    blob_t blob(value->get_content(), blob::btree_maxreflen);
    int64_t difference = (offset + val.size()) - blob.valuesize();
    if(difference > 0) {
        blob.append_region(oper.location.txn.get(), difference);
    }
    blob.write_from_string(val, oper.location.txn.get(), offset);

    return integer_result(blob.valuesize());
}

//COMMAND_1(integer, Strlen, string&)
integer_result redis_actor_t::Strlen(std::string &key) {
    std::string *string_result = get_string(key);
    return integer_result(string_result->size());
}

// Utility functions for hashes

struct hash_set_oper_t : set_oper_t {
    hash_set_oper_t(std::string &key, std::string &field, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp),
        nested_key(field),
        nested_sizer(btree->cache()->get_block_size())
    {
        // set_oper_t constructor finds this hash, here we find the field in the nested btree

        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        if(value == NULL) {
            // Hash doesn't exist, we create it on creation of the first field
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_HASH);
            value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
            value->get_root() = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_HASH) {
            // TODO in this case?
        }

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(value->get_root()));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, nested_key.key(), tstamp, &nested_loc);
    }

    ~hash_set_oper_t() {
        // Apply change to sub tree
        apply_keyvalue_change(&nested_sizer, &nested_loc, nested_key.key(), tstamp);
        
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
        virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(nested_loc.sb.get());
        value->get_root() = sb->get_root_block_id();
        // Parent change will be applied by parent destructor
    }

    keyvalue_location_t<redis_nested_string_value_t> nested_loc;

private:
    btree_key_buffer_t nested_key;
    value_sizer_t<redis_nested_string_value_t> nested_sizer;
};

std::string *redis_actor_t::hget_string(std::string &key, std::string &field) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    std::string *result = NULL;
    if(value == NULL) {
        //Error key doesn't exist
    } else if(value->get_redis_type() != REDIS_HASH) {
        //Error key isn't a string
    } else {
        value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

        got_superblock_t nested_superblock;
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(value->get_root()));
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        btree_key_buffer_t nested_key(field);
        keyvalue_location_t<redis_nested_string_value_t> nested_loc;

        find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, nested_key.key(), &nested_loc);

        redis_nested_string_value_t *value = nested_loc.value.get();
        if(value != NULL) {
            result = new std::string();
            blob_t blob(value->content, blob::btree_maxreflen);
            blob.read_to_string(*result, nested_loc.txn.get(), 0, blob.valuesize());
        } // else result remains NULL
    }

    return result;
}

//Hashes

//COMMAND_N(integer, hdel)
integer_result redis_actor_t::hdel(std::vector<std::string> &key_fields) {
    int deleted = 0;

    std::string key = key_fields[0];
    set_oper_t oper(key, btree, 1234);

    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

    if(oper.location.value.get() != NULL ||
       oper.location.value->get_redis_type() == REDIS_HASH) {
        
        redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
        block_id_t root = value->get_root();

        for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
                iter != key_fields.end(); ++iter) {
            btree_key_buffer_t field(*iter);

            // The nested superblock will be consumed for each field lookup
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);
            nested_superblock.txn = oper.location.txn;

            keyvalue_location_t<redis_nested_string_value_t> loc;
            find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, field.key(), oper.tstamp, &loc);

            redis_nested_string_value_t *nst_str = loc.value.get();
            if(nst_str != NULL) {
                // Delete the value
                blob_t blob(nst_str->content, blob::btree_maxreflen);
                blob.clear(oper.location.txn.get());

                scoped_malloc<redis_nested_string_value_t> null;
                loc.value.swap(null);

                apply_keyvalue_change(&nested_sizer, &loc, field.key(), oper.tstamp);

                // This delete may have changed the root which we need for the next operation
                root = reinterpret_cast<virtual_superblock_t *>(loc.sb.get())->get_root_block_id();

                deleted++;
            }
        }

        // If any of the deletes change the root we now have to make that change in the hash value
        value->get_root() = root;
    }

    return integer_result(deleted);
}

//COMMAND_2(integer, hexists, string&, string&)
integer_result redis_actor_t::hexists(std::string &key, std::string &field) {
    std::string *s = hget_string(key, field);
    if(s != NULL) delete s;
    return integer_result(!!s);
}

//COMMAND_2(bulk, hget, string&, string&)
bulk_result redis_actor_t::hget(std::string &key, std::string &field) {
    return bulk_result(boost::shared_ptr<std::string>(hget_string(key, field)));
}

//COMMAND_1(multi_bulk, hgetall, string&)
multi_bulk_result redis_actor_t::hgetall(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                std::string k = next.get().key;
                vals->push_back(k);

                boost::shared_array<char> val = next.get().value;
                blob_t blob(val.get(), blob::btree_maxreflen);
                std::string this_val(blob.valuesize(), '\0');
                blob.read_to_string(this_val, location.txn.get(), 0, blob.valuesize());
                vals->push_back(this_val);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);

}

//COMMAND_3(integer, hincrby, string&, string&, int)
integer_result redis_actor_t::hincrby(std::string &key, std::string &field, int by) {
    hash_set_oper_t oper(key, field, btree, 1234);

    int int_value = 0;
    std::string int_string;
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());
    } else {
        blob_t blob(value->content, blob::btree_maxreflen);
        blob.read_to_string(int_string, oper.location.txn.get(), 0, blob.valuesize());
        blob.clear(oper.location.txn.get());
        try {
            int_value = boost::lexical_cast<int64_t>(int_string);
        } catch(boost::bad_lexical_cast &) {
            return integer_result(redis_error("value is not an integer or out of range"));
        }
    }

    int_value += by;
    int_string = boost::lexical_cast<std::string>(int_value);

    blob_t blob(value->content, blob::btree_maxreflen);
    blob.append_region(oper.location.txn.get(), int_string.size());
    blob.write_from_string(int_string, oper.location.txn.get(), 0);
    
    return integer_result(int_value);
}

//COMMAND_1(multi_bulk, hkeys, string&)
multi_bulk_result redis_actor_t::hkeys(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                std::string k = next.get().key;
                vals->push_back(k);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);
}

//COMMAND_1(integer, hlen, string&)
integer_result redis_actor_t::hlen(std::string &key) {
    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    block_id_t root = value->get_root();

    boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> > nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

    store_key_t none_key;
    none_key.size = 0;

    boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
    slice_keys_iterator_t<redis_nested_string_value_t> tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

    int count = 0;
    while (true) {
        boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
        if(next) {
            count++;
        } else {
            break;
        }
    }

    return integer_result(count);
}

//COMMAND_N(multi_bulk, hmget)
multi_bulk_result redis_actor_t::hmget(std::vector<std::string> &key_fields) {
    std::string key = key_fields[0];
    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());
    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if((value != NULL) || (value->get_redis_type() == REDIS_HASH)) {
        
        block_id_t root = value->get_root();

        for(std::vector<std::string>::iterator iter = key_fields.begin() + 1;
                iter != key_fields.end(); ++iter) {
            btree_key_buffer_t field(*iter);

            // The nested superblock will be consumed for each field lookup
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);
            nested_superblock.txn = location.txn;

            keyvalue_location_t<redis_nested_string_value_t> loc;
            find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, field.key(), &loc);

            redis_nested_string_value_t *nst_str = loc.value.get();
            if(nst_str != NULL) {
                blob_t blob(nst_str->content, blob::btree_maxreflen);
                std::string res(blob.valuesize(), '\0');
                blob.read_to_string(res, location.txn.get(), 0, blob.valuesize());
                result->push_back(res);
            }
        }
    }

    return multi_bulk_result(result);
}

//COMMAND_N(status, hmset)
status_result redis_actor_t::hmset(std::vector<std::string> &key_fields_values) {
    std::string key = key_fields_values[0];
    set_oper_t oper(key, btree, 1234);

    value_sizer_t<redis_nested_string_value_t> nested_sizer(btree->cache()->get_block_size());

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
    if(value == NULL) {
        scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        oper.location.value.swap(smrsv);
        value = reinterpret_cast<redis_hash_value_t *>(oper.location.value.get());
        value->set_redis_type(REDIS_HASH);
        value->get_root() = NULL_BLOCK_ID;
    } else if(value->get_redis_type() != REDIS_HASH) {
        return redis_error("Hash operation attempted against a non-hash key");
    }
    
    block_id_t root = value->get_root();

    for(std::vector<std::string>::iterator iter = key_fields_values.begin() + 1;
            iter != key_fields_values.end(); ++iter) {
        btree_key_buffer_t field(*iter);

        // The nested superblock will be consumed for each field lookup
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = oper.location.txn;

        keyvalue_location_t<redis_nested_string_value_t> loc;
        find_keyvalue_location_for_write(&nested_sizer, &nested_superblock, field.key(), oper.tstamp, &loc);

        redis_nested_string_value_t *nst_str = loc.value.get();
        if(nst_str == NULL) {
            scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
            loc.value.swap(smrsv);
            nst_str = reinterpret_cast<redis_nested_string_value_t *>(loc.value.get());
        }

        blob_t blob(nst_str->content, blob::btree_maxreflen);

        // Clear old value
        blob.clear(loc.txn.get());
        blob.append_region(loc.txn.get(), iter->size());

        // Increment to the value of this field
        ++iter;
        blob.write_from_string(*iter, loc.txn.get(), 0);

        apply_keyvalue_change(&nested_sizer, &loc, field.key(), oper.tstamp);

        // This delete may have changed the root which we need for the next operation
        root = reinterpret_cast<virtual_superblock_t *>(loc.sb.get())->get_root_block_id();
    }

    // If any of the deletes change the root we now have to make that change in the hash value
    value->get_root() = root;

    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = OK;
    result->msg = "OK";
    return result;
}

//COMMAND_3(integer, hset, string&, string&, string&)
integer_result redis_actor_t::hset(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, field, btree, 1234);
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    int result = 0;
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());
        result = 1;
    }    

    blob_t blob(value->content, blob::btree_maxreflen);
    blob.clear(oper.location.txn.get());

    blob.append_region(oper.location.txn.get(), f_value.size());
    blob.write_from_string(f_value, oper.location.txn.get(),  0);

    return integer_result(result);
}

//COMMAND_3(integer, hsetnx, string&, string&, string&)
integer_result redis_actor_t::hsetnx(std::string &key, std::string &field, std::string &f_value) {
    hash_set_oper_t oper(key, field, btree, 1234);
    redis_nested_string_value_t *value = oper.nested_loc.value.get();
    int result = 0;
    if(value == NULL) {
        scoped_malloc<redis_nested_string_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        oper.nested_loc.value.swap(smrsv);
        value = reinterpret_cast<redis_nested_string_value_t *>(oper.nested_loc.value.get());

        blob_t blob(value->content, blob::btree_maxreflen);
        blob.append_region(oper.location.txn.get(), f_value.size());
        blob.write_from_string(f_value, oper.location.txn.get(),  0);

        result = 1;
    }    

    return integer_result(result);
}

//COMMAND_1(multi_bulk, hvals, string&)
multi_bulk_result redis_actor_t::hvals(std::string &key) {
    boost::shared_ptr<std::vector<std::string> > vals(new std::vector<std::string>());

    got_superblock_t superblock;
    get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);

    btree_key_buffer_t btree_key(key);

    keyvalue_location_t<redis_value_t> location;
    value_sizer_t<redis_value_t> sizer(btree->cache()->get_block_size());
    find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);

    redis_hash_value_t *value = reinterpret_cast<redis_hash_value_t *>(location.value.get());
    if(value != NULL) {
        block_id_t root = value->get_root();

        boost::shared_ptr<value_sizer_t<redis_nested_string_value_t> >
            nested_sizer(new value_sizer_t<redis_nested_string_value_t>(btree->cache()->get_block_size()));

        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        slice_keys_iterator_t<redis_nested_string_value_t>
            tree_iter(nested_sizer, location.txn, nested_btree_sb, btree->home_thread(),
            rget_bound_none, none_key, rget_bound_none, none_key);

        while (true) {
            boost::optional<key_value_pair_t<redis_nested_string_value_t> > next = tree_iter.next();
            if(next) {
                boost::shared_array<char> val = next.get().value;
                blob_t blob(val.get(), blob::btree_maxreflen);
                std::string this_val(blob.valuesize(), '\0');
                blob.read_to_string(this_val, location.txn.get(), 0, blob.valuesize());
                vals->push_back(this_val);
            } else {
                break;
            }
        }
    }

    return multi_bulk_result(vals);
}

// Sets

struct set_set_oper_t : set_oper_t {
    set_set_oper_t(std::string &key, btree_slice_t *btree, int64_t timestamp) :
        set_oper_t(key, btree, timestamp),
        nested_sizer(btree->cache()->get_block_size())
    { 
        redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        if(value == NULL) {
            scoped_malloc<redis_value_t> smrsv(MAX_BTREE_VALUE_SIZE);
            location.value.swap(smrsv);
            location.value->set_redis_type(REDIS_SET);
            value = reinterpret_cast<redis_set_value_t *>(location.value.get());
            value->get_root() = NULL_BLOCK_ID;
            value->get_sub_size() = 0;
        } else if(value->get_redis_type() != REDIS_SET) {
            // TODO in this case?
        }

        root = value->get_root();
    }

    bool add_member(std::string &member) {
        find_member mem(this, member);
        
        if(mem.loc.value.get() == NULL) {
            scoped_malloc<redis_nested_set_value_t> smrsv(0);
            mem.loc.value.swap(smrsv);
            mem.apply_change();
            return true;
        } // else this operation has no effect

        return false;
    }

    bool remove_member(std::string &member) {
        find_member mem(this, member);

        if(mem.loc.value.get() == NULL) {
            return false;
        }

        scoped_malloc<redis_nested_set_value_t> null;
        mem.loc.value.swap(null);
        mem.apply_change();
        return true;
    }

    void increment_size(int by) {
        reinterpret_cast<redis_set_value_t *>(location.value.get())->get_sub_size() += by;
    }

    ~set_set_oper_t() {
        // Reset root if changed
        redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        value->get_root() = root;
    }

protected:
    struct find_member {
        find_member(set_set_oper_t *ths_ptr, std::string &member):
            ths(ths_ptr),
            nested_key(member)
        {
            boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(ths->root));
            got_superblock_t nested_superblock;
            nested_superblock.sb.swap(nested_btree_sb);
            nested_superblock.txn = ths->location.txn;

            find_keyvalue_location_for_write(&ths->nested_sizer, &nested_superblock, nested_key.key(), ths->tstamp, &loc);
        }
        
        void apply_change() {
            apply_keyvalue_change(&ths->nested_sizer, &loc, nested_key.key(), ths->tstamp);
            virtual_superblock_t *sb = reinterpret_cast<virtual_superblock_t *>(loc.sb.get());
            ths->root = sb->get_root_block_id();
        }

        set_set_oper_t *ths;
        btree_key_buffer_t nested_key;
        keyvalue_location_t<redis_nested_set_value_t> loc;
    };

    block_id_t root;
    value_sizer_t<redis_nested_set_value_t> nested_sizer;
};

struct read_oper_t {
    read_oper_t(std::string &key, btree_slice_t *btree) :
        sizer(btree->cache()->get_block_size())
    {
        got_superblock_t superblock;
        get_btree_superblock(btree, rwi_read, order_token_t::ignore, &superblock);
        btree_key_buffer_t btree_key(key);
        find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);
    }

    keyvalue_location_t<redis_value_t> location;

protected:
    value_sizer_t<redis_value_t> sizer;
};

struct set_read_oper_t : read_oper_t {
    set_read_oper_t(std::string &key, btree_slice_t *btr) :
        read_oper_t(key, btr),
        btree(btr)
    {
        value = reinterpret_cast<redis_set_value_t *>(location.value.get());
        if(!value) {
            root = NULL_BLOCK_ID;
        } else if(value->get_redis_type() != REDIS_SET) {

        } else {
            root = value->get_root();
        }
    }

    bool contains_member(std::string &member) {
        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));
        got_superblock_t nested_superblock;
        nested_superblock.sb.swap(nested_btree_sb);
        nested_superblock.txn = location.txn;

        value_sizer_t<redis_nested_set_value_t> nested_sizer(sizer.block_size());
        btree_key_buffer_t nested_key(member);
        keyvalue_location_t<redis_nested_set_value_t> loc;
        find_keyvalue_location_for_read(&nested_sizer, &nested_superblock, nested_key.key(), &loc);
        return (loc.value.get() != NULL);
    }

    boost::shared_ptr<one_way_iterator_t<std::string> > iterator() {
        boost::shared_ptr<value_sizer_t<redis_nested_set_value_t> >
            sizer_ptr(new value_sizer_t<redis_nested_set_value_t>(sizer.block_size()));

        // We nest a slice_keys iterator inside a transform iterator
        store_key_t none_key;
        none_key.size = 0;

        boost::scoped_ptr<superblock_t> nested_btree_sb(new virtual_superblock_t(root));

        slice_keys_iterator_t<redis_nested_set_value_t> *tree_iter =
                new slice_keys_iterator_t<redis_nested_set_value_t>(sizer_ptr, location.txn,
                nested_btree_sb, btree->home_thread(), rget_bound_none, none_key, rget_bound_none, none_key);

        boost::shared_ptr<one_way_iterator_t<std::string > > transform_iter(
                new transform_iterator_t<key_value_pair_t<redis_nested_set_value_t>, std::string>(
                boost::bind(&transform_value, _1), tree_iter));

        return transform_iter;
    }

    redis_set_value_t *value;
    block_id_t root;

private:
    btree_slice_t *btree;

    static std::string transform_value(const key_value_pair_t<redis_nested_set_value_t> &pair) {
        return pair.key;
    }
};

//COMMAND_N(integer, sadd)
integer_result redis_actor_t::sadd(std::vector<std::string> &key_mems) {
    int count = 0;
    set_set_oper_t oper(key_mems[0], btree, 1234);
    for(std::vector<std::string>::iterator iter = key_mems.begin() + 1; iter != key_mems.end(); ++iter) {
        if(oper.add_member(*iter)) {
            count++;
        }
    }

    oper.increment_size(count);
    return count;
}

//COMMAND_1(integer, scard, string&)
integer_result redis_actor_t::scard(std::string &key) {
    set_read_oper_t oper(key, btree);
    redis_set_value_t *value = reinterpret_cast<redis_set_value_t *>(oper.location.value.get());
    if(value == NULL) return 0;
    return value->get_sub_size();
}

//COMMAND_N(multi_bulk, sdiff)
multi_bulk_result redis_actor_t::sdiff(std::vector<std::string> &keys) {
    merge_ordered_data_iterator_t<std::string> *right =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin() + 1; iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        right->add_mergee(set_iter);
    }

    set_read_oper_t oper(keys[0], btree);

    // Why put this into a merged data iterator first? Because the memory for the iterator returned
    // by oper.iterator is managed by a shared ptr it doesn't play well with the diff_filter_iterator
    boost::shared_ptr<one_way_iterator_t<std::string> > left_iter = oper.iterator();
    merge_ordered_data_iterator_t<std::string> *left =
        new merge_ordered_data_iterator_t<std::string>();
    left->add_mergee(left_iter);
    
    diff_filter_iterator_t<std::string> filter(left, right);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);

}

COMMAND_N(integer, sdiffstore)

//COMMAND_N(multi_bulk, sinter)
multi_bulk_result redis_actor_t::sinter(std::vector<std::string> &keys) {
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the repetition filter to get only results common to all keys
    repetition_filter_iterator_t<std::string> filter(merged, keys.size());

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);
}

COMMAND_N(integer, sinterstore)

//COMMAND_2(integer, sismember, string&, string&)
integer_result redis_actor_t::sismember(std::string &key, std::string &member) {
    set_read_oper_t oper(key, btree);
    return integer_result(oper.contains_member(member));
}

//COMMAND_1(multi_bulk, smembers, string&)
multi_bulk_result redis_actor_t::smembers(std::string &key) {
    set_read_oper_t oper(key, btree);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>);
    boost::shared_ptr<one_way_iterator_t<std::string> > iter = oper.iterator();
    while(true) {
        boost::optional<std::string> mem = iter->next();
        if(mem) {
            std::string bob = mem.get();
            result->push_back(bob);
        } else break;
    }

    return multi_bulk_result(result);
}

COMMAND_3(integer, smove, string&, string&, string&)
COMMAND_1(bulk, spop, string&)
COMMAND_1(bulk, srandmember, string&)

//COMMAND_N(integer, srem)
integer_result redis_actor_t::srem(std::vector<std::string> &key_members) {
    set_set_oper_t oper(key_members[0], btree, 1234);

    int count = 0;
    for(std::vector<std::string>::iterator iter = key_members.begin() + 1; iter != key_members.end(); ++iter) {
        if(oper.remove_member(*iter))
            ++count;
    }

    oper.increment_size(-count);
    return integer_result(count);
}

//COMMAND_N(multi_bulk, sunion)
multi_bulk_result redis_actor_t::sunion(std::vector<std::string> &keys) {
    // We can't declare this as a local variable because the reptition filter iterator
    // will try to delete it it it's destructor. It can't delete something that wasn't
    // malloched. What a waste. I hope there is a good reason for this elsewhere.
    merge_ordered_data_iterator_t<std::string> *merged =
        new merge_ordered_data_iterator_t<std::string>();

    // Add each set iterator to the merged data iterator
    for(std::vector<std::string>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
        set_read_oper_t oper(*iter, btree);
        boost::shared_ptr<one_way_iterator_t<std::string> > set_iter = oper.iterator();
        merged->add_mergee(set_iter);
    }
    
    // Use the unique filter to eliminate duplicates
    unique_filter_iterator_t<std::string> filter(merged);

    boost::shared_ptr<std::vector<std::string> > result(new std::vector<std::string>());
    while(true) {
        boost::optional<std::string> next = filter.next();
        if(!next) break;

        result->push_back(next.get());
    }

    return multi_bulk_result(result);
}

COMMAND_N(integer, sunionstore)
