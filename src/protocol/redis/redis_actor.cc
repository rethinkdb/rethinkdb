#include "protocol/redis/redis_actor.hpp"
#include "protocol/redis/redis.hpp"
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/blob.hpp"
#include "containers/buffer_group.hpp"
#include <boost/lexical_cast.hpp>
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
    // Bits 0-2 reserved for type field. Bit 3 indicates expiration. Bit 4-7 reserved for the various value types.
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

    size_t get_metadata_size() const {
        return sizeof(redis_value_t) + sizeof(uint32_t)*expiration_set();
    }
} __attribute__((__packed__));

struct redis_string_value_t : redis_value_t {
    char *get_content() {
       return reinterpret_cast<char *>(this + get_metadata_size());
    }

    char *get_content() const {
       return const_cast<char *>(reinterpret_cast<const char *>(this + get_metadata_size()));
    }

    int size(const block_size_t &bs) const {
        blob_t blob(get_content(), blob::btree_maxreflen);
        return get_metadata_size() + blob.refsize(bs);
    }

    void clear(transaction_t *txn) {
        blob_t blob(get_content(), blob::btree_maxreflen);
        blob.clear(txn);
    }
} __attribute__((__packed__));

template <>
class value_sizer_t<redis_value_t> {
public:
    value_sizer_t(block_size_t bs) : block_size_(bs) { }

    int size(const redis_value_t *value) const {
        switch(value->get_redis_type()) {
        case REDIS_STRING:
            return reinterpret_cast<const redis_string_value_t *>(this)->size(block_size_);
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
        
        assert(NULL);    
        return 0;
    }

    bool fits(const redis_value_t *value, int length_available) const {
        int value_size = size(value);
        return value_size <= length_available;
    }

    int max_possible_size() const {
        return MAX_BTREE_VALUE_SIZE;
    }

    block_magic_t btree_leaf_magic() const {
        block_magic_t magic = { { 'r', 'e', 'd', 's' } };
        return magic;
    }

    block_size_t block_size() const { return block_size_; }

protected:
    block_size_t block_size_;
};

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

private:
    got_superblock_t superblock;
    repli_timestamp_t tstamp;
    value_sizer_t<redis_value_t> sizer;
    btree_key_buffer_t btree_key;
};

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
    buffer_group_t store_out;
    boost::scoped_ptr<blob_acq_t> acq_out(new blob_acq_t);
    blob.expose_region(oper.location.txn.get(), rwi_read, 0, blob.valuesize(), &store_out, acq_out.get());

    int64_t val_size = blob.valuesize();
    std::string int_string(val_size, '\0');

    buffer_group_t str_buff_out;
    str_buff_out.add_buffer(val_size, const_cast<char *>(int_string.c_str()));
    buffer_group_copy_data(&str_buff_out, const_view(&store_out));

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

    buffer_group_t str_buff_in;
    str_buff_in.add_buffer(int_string.size(), const_cast<char *>(int_string.c_str()));

    buffer_group_t store_in;
    boost::scoped_ptr<blob_acq_t> acq_in(new blob_acq_t);
    blob.append_region(oper.location.txn.get(), int_string.size());
    blob.expose_region(oper.location.txn.get(), rwi_write, 0, blob.valuesize(), &store_in, acq_in.get());

    buffer_group_copy_data(&store_in, const_view(&str_buff_in));

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
    static std::string *result = NULL;
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
