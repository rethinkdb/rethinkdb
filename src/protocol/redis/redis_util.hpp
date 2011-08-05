#ifndef __PROTOCOL_REDIS_UTIL_H__
#define __PROTOCOL_REDIS_UTIL_H__

#include "protocol/redis/redis_types.hpp"
#include "protocol/redis/redis_actor.hpp"

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

status_result redis_error(const char *msg);

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


#endif /*__PROTOCOL_REDIS_UTIL_H__*/
