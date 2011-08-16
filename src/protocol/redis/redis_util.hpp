#ifndef __PROTOCOL_REDIS_UTIL_H__
#define __PROTOCOL_REDIS_UTIL_H__

#include "protocol/redis/redis_types.hpp"
#include "protocol/redis/redis_actor.hpp"
#include "protocol/redis/redis.hpp"
#include <boost/lexical_cast.hpp>

typedef repli_timestamp_t timestamp_t;

//These macros deliberately match those in the header file. They simply define an "unimplemented
//command" definition for the given command. When adding new commands we can simply copy and paste
//declaration from the header file to get a default implementation. Gradually these will go away
//as we actually define real implementations of the various redis commands.

#define WRITE(CNAME)\
redis_protocol_t::indicated_key_t redis_protocol_t::CNAME::get_keys() {return std::string("");}\
 \
std::vector<redis_protocol_t::write_t> redis_protocol_t::CNAME::shard(std::vector<redis_protocol_t::region_t> &regions) { \
    (void)regions; \
    return std::vector<write_t>(1, boost::shared_ptr<write_operation_t>(new CNAME(*this))); \
} \
 \
redis_protocol_t::write_response_t redis_protocol_t::CNAME::execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) { \
    (void)btree; \
    (void)timestamp; \
    (void)otok; \
    throw "Redis command " #CNAME " not yet implemented"; \
}

#define READ(CNAME)\
redis_protocol_t::indicated_key_t redis_protocol_t::CNAME::get_keys() {return std::string("");}\
 \
std::vector<redis_protocol_t::read_t> redis_protocol_t::CNAME::shard(std::vector<redis_protocol_t::region_t> &regions) { \
    (void)regions; \
    return std::vector<read_t>(1, boost::shared_ptr<read_operation_t>(new CNAME(*this))); \
} \
 \
std::vector<redis_protocol_t::read_t> redis_protocol_t::CNAME::parallelize(int optimal_factor) { \
    (void)optimal_factor; \
    return std::vector<read_t>(1, boost::shared_ptr<read_operation_t>(new CNAME(*this))); \
} \
 \
redis_protocol_t::read_response_t redis_protocol_t::CNAME::execute(btree_slice_t *btree, order_token_t otok) { \
    (void)btree; \
    (void)otok; \
    throw "Redis command " #CNAME " not yet implemented"; \
}

// Macros to make some of these implementations a little less tedius

// Assumes the key (or keys) is simply the first argument to the command. This is true for almost all commands
#define KEYS(CNAME) redis_protocol_t::indicated_key_t redis_protocol_t::CNAME::get_keys() { \
    return one; \
}

// Simple shard behavior that merely returns one copy of this operation. Useful when the operation will only
// touch one key (and thus touch only one shard) which is most of them
#define SHARD_W(CNAME) std::vector<redis_protocol_t::write_t> \
        redis_protocol_t::CNAME::shard(std::vector<redis_protocol_t::region_t> &regions) { \
    (void)regions; \
    return std::vector<write_t>(1, boost::shared_ptr<write_operation_t>(new CNAME(*this))); \
}

#define SHARD_R(CNAME) std::vector<redis_protocol_t::read_t> \
        redis_protocol_t::CNAME::shard(std::vector<redis_protocol_t::region_t> &regions) { \
    (void)regions; \
    return std::vector<read_t>(1, boost::shared_ptr<read_operation_t>(new CNAME(*this))); \
}

// Similar for parallelize
#define PARALLEL(CNAME) std::vector<redis_protocol_t::read_t> \
        redis_protocol_t::CNAME::parallelize(int optimal_factor) { \
    (void)optimal_factor; \
    return std::vector<read_t>(1, boost::shared_ptr<read_operation_t>(new CNAME(*this))); \
} 

#define EXECUTE_R(CNAME) redis_protocol_t::read_response_t redis_protocol_t::CNAME::execute(btree_slice_t *btree, order_token_t otok)

#define EXECUTE_W(CNAME) redis_protocol_t::write_response_t redis_protocol_t::CNAME::execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok)

struct set_oper_t {
    set_oper_t(std::string &key, btree_slice_t *btree, timestamp_t timestamp_, order_token_t otok) :
        sizer(btree->cache()->get_block_size()),
        btree_key(key),
        timestamp(timestamp_)
    {
        // Get the superblock that represents our write transaction
        get_btree_superblock(btree, rwi_write, 1, timestamp, otok, &superblock);
        find_keyvalue_location_for_write(&sizer, &superblock, btree_key.key(), timestamp, &location);
    }

    ~set_oper_t() {
        apply_keyvalue_change(&sizer, &location, btree_key.key(), timestamp);
    }

    bool del() {
        if(location.value.get() == NULL) return false;

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
        default:
            assert(0);
            break;
        }

        scoped_malloc<redis_value_t> null;
        location.value.swap(null);
        return true;
    }

    keyvalue_location_t<redis_value_t> location;

protected:
    got_superblock_t superblock;
    value_sizer_t<redis_value_t> sizer;
    btree_key_buffer_t btree_key;
    timestamp_t timestamp;
};

struct read_oper_t {
    read_oper_t(std::string &key, btree_slice_t *btree, order_token_t otok) :
        sizer(btree->cache()->get_block_size())
    {
        got_superblock_t superblock;
        get_btree_superblock(btree, rwi_read, otok, &superblock);
        btree_key_buffer_t btree_key(key);
        find_keyvalue_location_for_read(&sizer, &superblock, btree_key.key(), &location);
    }

    bool exists() {
        return (location.value.get() != NULL);
    }

    keyvalue_location_t<redis_value_t> location;

protected:
    value_sizer_t<redis_value_t> sizer;
};


template <typename T>
int incr_loc(keyvalue_location_t<T> &loc, int by) {
    int int_value = 0;
    std::string int_string;
    T *value = loc.value.get();
    if(value == NULL) {
        scoped_malloc<T> smrsv(MAX_BTREE_VALUE_SIZE);
        memset(smrsv.get(), 0, MAX_BTREE_VALUE_SIZE);
        loc.value.swap(smrsv);
        value = loc.value.get();
    } else {
        blob_t blob(value->get_content(), blob::btree_maxreflen);
        blob.read_to_string(int_string, loc.txn.get(), 0, blob.valuesize());
        blob.clear(loc.txn.get());
        try {
            int_value = boost::lexical_cast<int64_t>(int_string);
        } catch(boost::bad_lexical_cast &) {
            // TODO Error cond
        }
    }

    int_value += by;
    int_string = boost::lexical_cast<std::string>(int_value);

    blob_t blob(value->get_content(), blob::btree_maxreflen);
    blob.append_region(loc.txn.get(), int_string.size());
    blob.write_from_string(int_string, loc.txn.get(), 0);

    return int_value;
}

status_result redis_error(const char *msg);
status_result redis_ok();

// What is this crazy contraption? Well origionally it was supposed to be a simple convienience function to
// return an integer result wrapped in a write_t or read_t. But I didn't want to have to create two versions
// (one for read_t and one for write_t) so this is what I had to do instead, create a functor with overloaded
// cast operators
struct int_response {
    int_response(int val_) : val(val_) { }

    operator redis_protocol_t::read_response_t () {
        return redis_protocol_t::read_response_t(new redis_protocol_t::integer_result_t(val));
    }

    operator redis_protocol_t::write_response_t () {
        return redis_protocol_t::write_response_t(new redis_protocol_t::integer_result_t(val));
    }

private:
    int val;
};

#endif /*__PROTOCOL_REDIS_UTIL_H__*/
