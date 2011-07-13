#include "protocol/redis/redis.hpp"
#include <iostream>
#include <string>
#include <vector>

redis_interface_t::redis_interface_t(get_store_t *get_store_p, set_store_interface_t *set_store_p) :
    get_store(get_store_p),
    set_store(set_store_p)
{ }

redis_interface_t::~redis_interface_t() {}

//This class is nothing more than a hack to make implementing a default "not implemented" definition
//for as yet unimplemented redis commands much easier. It helps us use the type system to
//automatically select the right "null" result for the given command.
struct unimplemented_result {
    unimplemented_result() { }
    operator status_result () {
        boost::shared_ptr<status_result_struct> result(new status_result_struct);
        result->status = ERROR;
        result->msg = (const char *)("Unimplemented");
        return result;
    }

    operator integer_result () {
        return 0;
    }

    operator bulk_result () {
        boost::shared_ptr<std::string> res(new std::string("Unimplemented"));
        return res;
    }

    operator multi_bulk_result () {
        multi_bulk_result res(new std::vector<std::string>());
        res->push_back(std::string("Unimplemented"));
        return res;
    }
};

//These macros deliberately match those in the header file. They simply define an "unimplemented
//command" definition for the given command. When adding new commands we can simply copy and paste
//declaration from the header file to get a default implementation. Gradually these will go away
//as we actually define real implementations of the various redis commands.
#define COMMAND_0(RETURN, CNAME) RETURN##_result \
redis_interface_t::CNAME() \
{ \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_1(RETURN, CNAME, ARG_TYPE_ONE) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one) \
{ \
    (void)one; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_2(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two) \
{ \
    (void)one; \
    (void)two; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_3(RETURN, CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE) RETURN##_result \
redis_interface_t::CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two, ARG_TYPE_THREE three) \
{ \
    (void)one; \
    (void)two; \
    (void)three; \
    std::cout << "Redis command \"" << #CNAME << "\" has not been implemented" << std::endl; \
    return unimplemented_result(); \
}

#define COMMAND_N(RETURN, CNAME) RETURN##_result \
redis_interface_t::CNAME(std::vector<std::string> &one) \
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
