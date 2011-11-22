#ifndef __PROTOCOL_REDIS_REDIS_HPP__
#define __PROTOCOL_REDIS_REDIS_HPP__

#include "arch/arch.hpp"
#include "utils.hpp"
#include "redis/redis_types.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <deque>
#include <vector>
#include <string>

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
    typedef boost::shared_ptr<read_result_t> read_response_t;
    typedef boost::shared_ptr<write_result_t> write_response_t;

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

    class read_t {
    public:
        read_t(read_operation_t *ptr) : op(ptr) { }
        read_t() { }
        region_t get_region();
        read_t shard(region_t region);
        read_response_t unshard(const std::vector<read_response_t> &bits, temporary_cache_t *cache);
        boost::shared_ptr<read_operation_t> op;
    };

    // Base class for redis read operations
    struct read_operation_t {
        virtual ~read_operation_t(){};
        virtual indicated_key_t get_keys() = 0;
        virtual read_t shard(region_t mask) = 0;
        virtual read_response_t execute(btree_slice_t *btree, order_token_t otok) = 0;

    private:
        std::string one;
    };

    struct write_t {
    public:
        write_t(write_operation_t *ptr) : op(ptr) { }
        write_t() { }
        region_t get_region();
        write_t shard(region_t region);
        write_response_t unshard(const std::vector<write_response_t> &bits, temporary_cache_t *cache);
        boost::shared_ptr<write_operation_t> op;
    };

    // Base class for redis write operations
    struct write_operation_t {
        virtual ~write_operation_t(){};
        virtual indicated_key_t get_keys() = 0;
        virtual write_t shard(region_t mask) = 0;
        virtual write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok) = 0;
    };

    struct read_result_t {
        virtual ~read_result_t(){};
        
        // Reduces other into this. Used to allow subclasses to define unshard/unparallelize behavior.
        virtual void deshard(const void *other) = 0;

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

    // Redis result types as read_response_t's and write_response_t's

    // Integer response classes

    // Base integer result class, by default it reduces by choosing the first value
    // This is, for example, the expected behavior for the innumerable 1 or 0 integer responses
    struct integer_result_t : read_result_t, write_result_t {
        integer_result_t(int val) : value(val) {;}
        virtual void deshard(const void *other) {(void)other;}
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
                unreachable();
            }
        }
        virtual void deshard(const void *other) {(void)other;}
        virtual redis_return_type get_result() { return value; }

        std::string value;
    };

    // Multi-bulk response base class
    struct multi_bulk_result_t : read_result_t, write_result_t {
        multi_bulk_result_t(std::vector<std::string> &val) : value(val) {;}
        multi_bulk_result_t(std::deque<std::string> &val) : value(val.begin(), val.end()) {;}
        virtual void deshard(const void *other) {(void)other;}
        virtual redis_return_type get_result() { return value; }

        std::vector<std::string> value;
    };

    // Status response classes
    struct msg_result_t : read_result_t, write_result_t {
        virtual void deshard(const void *other) {(void)other;}
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
        virtual write_t shard(region_t region); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    };

    #define WRITE_1(CNAME, ARG_TYPE_ONE)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual write_t shard(region_t region); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
    };

    #define WRITE_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_) : one(one_), two(two_) { } \
        virtual indicated_key_t get_keys(); \
        virtual write_t shard(region_t region); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
    };

    #define WRITE_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
    struct CNAME : write_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_) : one(one_), two(two_), three(three_) { } \
        virtual indicated_key_t get_keys(); \
        virtual write_t shard(region_t region); \
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
        virtual write_t shard(region_t region); \
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
        virtual read_t shard(region_t region); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    };

    #define READ__1(CNAME, ARG_TYPE_ONE)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual read_t shard(region_t region); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
    };

    #define READ__2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_) : one(one_), two(two_) { } \
        virtual indicated_key_t get_keys(); \
        virtual read_t shard(region_t region); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        ARG_TYPE_ONE one; \
        ARG_TYPE_TWO two; \
    };

    #define READ__3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
    struct CNAME : read_operation_t { \
        CNAME(ARG_TYPE_ONE one_, ARG_TYPE_TWO two_, ARG_TYPE_THREE three_) : one(one_), two(two_), three(three_) { } \
        virtual indicated_key_t get_keys(); \
        virtual read_t shard(region_t region); \
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
        virtual read_t shard(region_t region); \
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
        virtual write_t shard(region_t region); \
        virtual redis_protocol_t::write_response_t execute(btree_slice_t *btree, timestamp_t timestamp, order_token_t otok); \
    private: \
        std::vector<std::string> one; \
    };

    #define READ__N(CNAME) \
    struct CNAME : read_operation_t { \
        CNAME(std::vector<std::string> &one_) : one(one_) { } \
        virtual indicated_key_t get_keys(); \
        virtual read_t shard(region_t region); \
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok); \
    private: \
        std::vector<std::string> one; \
    };

    //KEYS
    WRITE_N(del)
    READ__1(exists, std::string&)
    WRITE_2(expire, std::string&, unsigned)
    WRITE_2(expireat, std::string&, unsigned)
    READ__1(keys, std::string&)
    WRITE_2(move, std::string&, std::string&)
    WRITE_1(persist, std::string&)
    READ__0(randomkey)
    READ__1(rename_get_type, std::string&)
    READ__1(ttl, std::string&)
    READ__1(type, std::string&)

    //STRINGS
    WRITE_2(append, std::string&, std::string&)
    WRITE_1(decr, std::string&)
    WRITE_2(decrby, std::string&, int)
    READ__1(get, std::string&)
    READ__2(getbit, std::string&, unsigned)
    READ__3(getrange, std::string&, int, int)
    WRITE_2(getset, std::string&, std::string&)
    WRITE_1(incr, std::string&)
    WRITE_2(incrby, std::string&, int)
    READ__N(mget)
    WRITE_N(mset)
    WRITE_N(msetnx)
    WRITE_2(set, std::string&, std::string&)
    WRITE_3(setbit, std::string&, unsigned, unsigned)
    WRITE_3(setex, std::string&, unsigned, std::string&)
    WRITE_2(setnx, std::string&, std::string&)
    WRITE_3(setrange, std::string&, unsigned, std::string&)
    READ__1(Strlen, std::string&)

    //Hashes
    WRITE_N(hdel)
    READ__2(hexists, std::string&, std::string&)
    READ__2(hget, std::string&, std::string&)
    READ__1(hgetall, std::string&)
    WRITE_3(hincrby, std::string&, std::string&, int)
    READ__1(hkeys, std::string&)
    READ__1(hlen, std::string&)
    READ__N(hmget)
    WRITE_N(hmset)
    WRITE_3(hset, std::string&, std::string&, std::string&)
    WRITE_3(hsetnx, std::string&, std::string&, std::string&)
    READ__1(hvals, std::string&)

    // Sets
    WRITE_N(sadd)
    READ__1(scard, std::string&)
    READ__N(sdiff)
    READ__N(sinter)
    READ__2(sismember, std::string&, std::string&)
    READ__1(smembers, std::string&)
    WRITE_1(spop, std::string&)
    READ__1(srandmember, std::string&)
    WRITE_N(srem)
    READ__N(sunion)
    WRITE_N(sunionstore)

    // Lists
    WRITE_N(blpop)
    WRITE_N(brpop)
    WRITE_N(brpoplpush)
    READ__2(lindex, std::string&, int)
    WRITE_4(linsert, std::string&, std::string&, std::string&, std::string&)
    READ__1(llen, std::string&)
    WRITE_1(lpop, std::string&)
    WRITE_N(lpush)
    WRITE_2(lpushx, std::string&, std::string&)
    READ__3(lrange, std::string&, int, int)
    WRITE_3(lrem, std::string&, int, std::string&)
    WRITE_3(lset, std::string&, int, std::string&)
    WRITE_3(ltrim, std::string&, int, int)
    WRITE_1(rpop, std::string&)
    WRITE_N(rpush)
    WRITE_2(rpushx, std::string&, std::string&)

    // Sorted Sets
    WRITE_N(zadd)
    READ__1(zcard, std::string)
    READ__3(zcount, std::string, float, float)
    WRITE_3(zincrby, std::string, float, std::string)
    WRITE_N(zinterstore)

    struct zrange : read_operation_t {
        zrange(std::string one_, int two_, int three_, bool four_, bool five_) : key(one_), start(two_), stop(three_), with_scores(four_), reverse(five_) { }
        virtual indicated_key_t get_keys();
        virtual read_t shard(region_t region);
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
        virtual read_t shard(region_t region);
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
        virtual read_t shard(region_t region);
        virtual redis_protocol_t::read_response_t execute(btree_slice_t *btree, order_token_t otok);

    private:
        std::string key;
        std::string member;
        bool reverse;
    };

    WRITE_N(zrem)
    WRITE_3(zremrangebyrank, std::string, int, int)
    WRITE_3(zremrangebyscore, std::string, float, float)
    READ__N(zrevrangebyscore)
    READ__2(zrevrank, std::string, std::string)
    READ__2(zscore, std::string, std::string)
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

    class backfill_chunk_t {
        /* stub */
    };
};

class dummy_redis_store_view_t : public store_view_t<redis_protocol_t> {
public:
    dummy_redis_store_view_t(key_range_t, btree_slice_t *);

    boost::shared_ptr<store_view_t<redis_protocol_t>::read_transaction_t> begin_read_transaction(signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    boost::shared_ptr<store_view_t<redis_protocol_t>::write_transaction_t> begin_write_transaction(signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    class transaction_t : public virtual store_view_t<redis_protocol_t>::write_transaction_t {
    public:
        transaction_t(dummy_redis_store_view_t *parent_);

        region_map_t<redis_protocol_t, binary_blob_t> get_metadata(
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        redis_protocol_t::read_response_t read(
                const redis_protocol_t::read_t &,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        void send_backfill(
                const region_map_t<redis_protocol_t, state_timestamp_t> &start_point,
                const boost::function<void(redis_protocol_t::backfill_chunk_t)> &chunk_fun,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);

        void set_metadata(
            const region_map_t<redis_protocol_t, binary_blob_t> &new_metadata)
            THROWS_NOTHING;

        redis_protocol_t::write_response_t write(
                const redis_protocol_t::write_t &write,
                transition_timestamp_t timestamp)
                THROWS_NOTHING;

        void receive_backfill(
                const redis_protocol_t::backfill_chunk_t &chunk_fun,
                signal_t *interruptor)
                THROWS_ONLY(interrupted_exc_t);
    private:
        dummy_redis_store_view_t *parent;
    };

    region_map_t<redis_protocol_t, binary_blob_t> metadata;
    btree_slice_t *btree;
};

#endif /* __PROTOCOL_REDIS_REDIS_HPP__ */
