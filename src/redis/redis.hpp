#ifndef __PROTOCOL_REDIS_REDIS_HPP__
#define __PROTOCOL_REDIS_REDIS_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "redis/redis_types.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>
#include <string>
using std::string;

// For key ranges, we'll split this when I figure stuff out
#include "memcached/protocol.hpp"


struct redis_protocol_t {

    // Dummy namespace_interface_t expects a temporary cache object. Why?
    struct temporary_cache_t {};

    typedef transition_timestamp_t timestamp_t;

    struct read_operation_t;
    struct write_operation_t;
    struct read_result_t;
    struct write_result_t;

    typedef key_range_t region_t;
    typedef boost::shared_ptr<read_operation_t> read_t;
    typedef boost::shared_ptr<write_operation_t> write_t;

    struct read_response_t : boost::shared_ptr<read_result_t> {
        read_response_t(read_result_t *ptr) : boost::shared_ptr<read_result_t>(ptr) {;}
        read_response_t() : boost::shared_ptr<read_result_t>() {;}
        static read_response_t unshard(std::vector<read_response_t> &responses);
        static read_response_t unparallelize(std::vector<read_response_t> &responses);
    };

    struct write_response_t : boost::shared_ptr<write_result_t> {
        write_response_t(write_result_t *ptr) : boost::shared_ptr<write_result_t>(ptr) {;}
        write_response_t() : boost::shared_ptr<write_result_t>() {;}
        static write_response_t unshard(std::vector<write_response_t> &response);
    };

    struct status_result {
        status_result(const char *msg_) : msg(msg_) {;}
        const char *msg;
    };

    struct error_result {
        error_result(const char *msg_) : msg(msg_) {;}
        const char *msg;
    };

    struct nil_result {
        // Empty
    };

    typedef boost::variant<status_result, error_result, int, std::string, nil_result, std::vector<std::string> >
        redis_return_type;
    
    typedef boost::variant<std::vector<std::string>, std::string> indicated_key_t;

    // Base class for redis read operations
    struct read_operation_t {
        virtual ~read_operation_t(){};
        region_t get_region();
        virtual indicated_key_t get_keys() = 0;
        virtual std::vector<read_t> shard(std::vector<region_t> &regions) = 0;
        virtual std::vector<read_t> parallelize(int optimal_factor) = 0;
        virtual read_response_t execute(btree_slice_t *btree, order_token_t otok) = 0;

        // TODO Why does dummy namespace_interface_t now expect an unshard here?
        read_response_t unshard(std::vector<read_response_t> &responses, temporary_cache_t *cache) {
            (void)cache;
            return responses[0];
        }
    };

    // Base class for redis write operations
    struct write_operation_t {
        virtual ~write_operation_t(){};
        region_t get_region();
        virtual indicated_key_t get_keys() = 0;
        virtual std::vector<write_t> shard(std::vector<region_t> &regions) = 0;
        virtual write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) = 0;

        // TODO Why does dummy namespace_interface_t now expect an unshard here?
        write_response_t unshard(std::vector<write_response_t> &responses, temporary_cache_t *cache) {
            (void)cache;
            return responses[0];
        }
    };

    struct read_result_t {
        virtual ~read_result_t(){};
        
        // Reduces other into this. Used to allow subclasses to define unshard/unparallelize behavior.
        virtual void deshard(const void *other) = 0;
        virtual void deparallelize(const void *other) = 0;

        // Returns the actual value of this result
        virtual redis_return_type get_result() = 0;

    };

    struct write_result_t {
        virtual ~write_result_t(){};

        // Reduces other into this. Used to allow subclasses to define unshard behavior.
        virtual void deshard(const void *other) = 0;

        // Returns the actual value of this result
        virtual redis_return_type get_result() = 0;
    };

    struct store_t {
        static void create(serializer_t *ser, region_t region);
        store_t(serializer_t *ser, region_t region);
        ~store_t();
        region_t get_region();
        read_response_t read(read_t &read, order_token_t otok, signal_t *interruptor);
        write_response_t write(write_t &write, timestamp_t timestamp, order_token_t otok, signal_t *interruptor);
        // TODO plus others...

    private:
        mirrored_cache_config_t cache_config;
        cache_t cache;
        btree_slice_t btree;
        region_t region;
    };

    // Redis result types as read_response_t's and write_response_t's

    // Integer response classes

    // Base integer result class, by default it reduces by choosing the first value
    // This is, for example, the expected behavior for the innumerable 1 or 0 integer responses
    struct integer_result_t : read_result_t, write_result_t {
        integer_result_t(int val) : value(val) {;}
        virtual void deshard(const void *other) {(void)other;}
        virtual void deparallelize(const void *other) {(void)other;}
        virtual redis_return_type get_result() { return value; }

        int value;
    };

    // Example of integer_result class, this takes the max integer value on reduction
    struct max_integer_t : integer_result_t {
        max_integer_t(int val) : integer_result_t(val) {;}
        virtual void deshard(const void *other) {
            const integer_result_t *oth = reinterpret_cast<const integer_result_t *>(other);
            if(oth->value > value) value = oth->value;
        }
    };

    // Sums the values on reduction
    struct sum_integer_t : integer_result_t {
        sum_integer_t(int val) : integer_result_t(val) {;}
        virtual void deshard(const void *other) {
            const integer_result_t *oth = reinterpret_cast<const integer_result_t *>(other);
            value += oth->value; 
        }

        virtual void deparallelize(const void *other) {
            (void)other;
        }
    };

    // Bulk response classes

    // Bulk response base class
    struct bulk_result_t : read_result_t, write_result_t {
        bulk_result_t(std::string &val) : value(val) {;}
        // Create a result string from a float value
        bulk_result_t(float val) {
            try {
                value = boost::lexical_cast<std::string>(val);
            } catch(boost::bad_lexical_cast &) {
                assert(0);
            }
        }
        virtual void deshard(const void *other) {(void)other;}
        virtual void deparallelize(const void *other) {(void)other;}
        virtual redis_return_type get_result() { return value; }

        std::string value;
    };

    // Multi-bulk response base class
    struct multi_bulk_result_t : read_result_t, write_result_t {
        multi_bulk_result_t(std::vector<std::string> &val) : value(val) {;}
        multi_bulk_result_t(std::deque<std::string> &val) : value(val.begin(), val.end()) {;}
        virtual void deshard(const void *other) {(void)other;}
        virtual void deparallelize(const void *other) {(void)other;}
        virtual redis_return_type get_result() { return value; }

        std::vector<std::string> value;
    };

    // Status response classes
    struct msg_result_t : read_result_t, write_result_t {
        virtual void deshard(const void *other) {(void)other;}
        virtual void deparallelize(const void *other) {(void)other;}
        virtual redis_return_type get_result() = 0;
    };

    struct ok_result_t : msg_result_t {
        ok_result_t() : value("OK") {;}
        ok_result_t(const char *msg) : value(msg) {;}
        virtual redis_return_type get_result() {return value;}

        status_result value;
    };

    struct error_result_t : msg_result_t {
        error_result_t(const char *msg) : value(msg) {;}
        virtual redis_return_type get_result() {return value;}

        error_result value;
    };

    // Redis commands as read_t's and write_t's 

    #define WRITE_0(CNAME)\
    struct CNAME : write_operation_t { \
        CNAME() { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    };

    #define WRITE_1(CNAME, ARG_TYPE_ONE)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
    };

    #define WRITE_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_) : one(one_), two(two_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
    };

    #define WRITE_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_) : one(one_), two(two_), three(three_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
        ARG_TYPE_THREE three; \
    };

    #define WRITE_4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_, ARG_TYPE_FOUR four_) : one(one_), two(two_), three(three_), four(four_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
        ARG_TYPE_THREE three; \
        ARG_TYPE_FOUR four; \
    };
    
    #define READ__0(CNAME)\
    struct CNAME : read_operation_t { \
        CNAME() { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    };

    #define READ__1(CNAME, ARG_TYPE_ONE)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
    };

    #define READ__2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_) : one(one_), two(two_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
    };

    #define READ__3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_) : one(one_), two(two_), three(three_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
        ARG_TYPE_THREE three; \
    };

    #define READ__4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_, ARG_TYPE_FOUR four_) : one(one_), two(two_), three(three_), four(four_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
        ARG_TYPE_THREE three; \
        ARG_TYPE_FOUR four; \
    };


    #define WRITE_N(CNAME) \
    struct CNAME : write_operation_t { \
        CNAME(std::vector<std::string> &one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<write_t> shard(std::vector<region_t> &regions); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        std::vector<std::string> one; \
    };

    #define READ__N(CNAME) \
    struct CNAME : read_operation_t { \
        CNAME(std::vector<std::string> &one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual std::vector<read_t> shard(std::vector<region_t> &regions); \
        virtual std::vector<read_t> parallelize(int optimal_factor); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        std::vector<std::string> one; \
    };

    //KEYS
    WRITE_N(del)
    READ__1(exists, string&)
    WRITE_2(expire, string&, unsigned)
    WRITE_2(expireat, string&, unsigned)
    READ__1(keys, string&)
    WRITE_2(move, string&, string&)
    WRITE_1(persist, string&)
    READ__0(randomkey)
    READ__1(rename_get_type, string&)
    READ__1(ttl, string&)
    READ__1(type, string&)

    //STRINGS
    WRITE_2(append, string&, string&)
    WRITE_1(decr, string&)
    WRITE_2(decrby, string&, int)
    READ__1(get, string&)
    READ__2(getbit, string&, unsigned)
    READ__3(getrange, string&, int, int)
    WRITE_2(getset, string&, string&)
    WRITE_1(incr, string&)
    WRITE_2(incrby, string&, int)
    READ__N(mget)
    WRITE_N(mset)
    WRITE_N(msetnx)
    WRITE_2(set, string&, string&)
    WRITE_3(setbit, string&, unsigned, unsigned)
    WRITE_3(setex, string&, unsigned, string&)
    WRITE_2(setnx, string&, string&)
    WRITE_3(setrange, string&, unsigned, string&)
    READ__1(Strlen, string&)

    //Hashes
    WRITE_N(hdel)
    READ__2(hexists, string&, string&)
    READ__2(hget, string&, string&)
    READ__1(hgetall, string&)
    WRITE_3(hincrby, string&, string&, int)
    READ__1(hkeys, string&)
    READ__1(hlen, string&)
    READ__N(hmget)
    WRITE_N(hmset)
    WRITE_3(hset, string&, string&, string&)
    WRITE_3(hsetnx, string&, string&, string&)
    READ__1(hvals, string&)

    // Sets
    WRITE_N(sadd)
    READ__1(scard, string&)
    READ__N(sdiff)
    READ__N(sinter)
    READ__2(sismember, string&, string&)
    READ__1(smembers, string&)
    WRITE_1(spop, string&)
    READ__1(srandmember, string&)
    WRITE_N(srem)
    READ__N(sunion)
    WRITE_N(sunionstore)

    // Lists
    WRITE_N(blpop)
    WRITE_N(brpop)
    WRITE_N(brpoplpush)
    READ__2(lindex, string&, int)
    WRITE_4(linsert, string&, string&, string&, string&)
    READ__1(llen, string&)
    WRITE_1(lpop, string&)
    WRITE_N(lpush)
    WRITE_2(lpushx, string&, string&)
    READ__3(lrange, string&, int, int)
    WRITE_3(lrem, string&, int, string&)
    WRITE_3(lset, string&, int, string&)
    WRITE_3(ltrim, string&, int, int)
    WRITE_1(rpop, string&)
    WRITE_N(rpush)
    WRITE_2(rpushx, string&, string&)

    // Sorted Sets
    WRITE_N(zadd)
    READ__1(zcard, string)
    READ__3(zcount, string, float, float)
    WRITE_3(zincrby, string, float, string)
    WRITE_N(zinterstore)

    struct zrange : read_operation_t {
        zrange(std::string one_, int two_, int three_, bool four_, bool five_) : key(one_), start(two_), stop(three_), with_scores(four_), reverse(five_) { }
        virtual indicated_key_t get_keys();
        virtual std::vector<read_t> shard(std::vector<region_t> &regions);
        virtual std::vector<read_t> parallelize(int optimal_factor);
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok);
    private:
        std::string key;
        int start;
        int stop;
        bool with_scores;
        bool reverse;
    };

    struct zrangebyscore : read_operation_t {
        zrangebyscore(std::string one_, std::string two_, std::string three_, bool four_,
                bool five_, unsigned six_, unsigned seven_, bool eight_)
        : key(one_), min(two_), max(three_), with_scores(four_), limit(five_),
          offset(six_), count(seven_), reverse(eight_)
        { }
        virtual indicated_key_t get_keys();
        virtual std::vector<read_t> shard(std::vector<region_t> &regions);
        virtual std::vector<read_t> parallelize(int optimal_factor);
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok);
    private:
        std::string key;
        std::string min;
        std::string max;
        bool with_scores;
        bool limit;
        unsigned offset;
        unsigned count;
        bool reverse;
    };

    struct zrank : read_operation_t {
        zrank(std::string one_, std::string two_, bool three_) : key(one_), member(two_), reverse(three_) {;}
        virtual indicated_key_t get_keys();
        virtual std::vector<read_t> shard(std::vector<region_t> &regions);
        virtual std::vector<read_t> parallelize(int optimal_factor);
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok);

    private:
        std::string key;
        std::string member;
        bool reverse;
    };

    WRITE_N(zrem)
    WRITE_3(zremrangebyrank, string, int, int)
    WRITE_3(zremrangebyscore, string, float, float)
    READ__N(zrevrangebyscore)
    READ__2(zrevrank, string, string)
    READ__2(zscore, string, string)
    WRITE_N(zunionstore)
    
    #undef WRITE_0
    #undef WRITE_1
    #undef WRITE_2
    #undef WRITE_3
    #undef WRITE_4
    #undef READ__0
    #undef READ__1
    #undef READ__2
    #undef READ__3
    #undef WRITE_N
    #undef READ__N
};

#endif /* __PROTOCOL_REDIS_REDIS_HPP__ */
