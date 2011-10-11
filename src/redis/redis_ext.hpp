#include "redis/redis.hpp"
#include "arch/runtime/coroutines.hpp"
#include "concurrency/wait_any.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/variant/get.hpp>
#include <map>
#include <list>

#define CMD_0(CNAME) \
redis_protocol_t::redis_return_type CNAME() { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(); \
    return exec(oper); \
}

#define CMD_1(CNAME, ARG_TYPE_ONE)\
redis_protocol_t::redis_return_type CNAME(ARG_TYPE_ONE one) { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(one); \
    return exec(oper); \
}

#define CMD_2(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO)\
redis_protocol_t::redis_return_type CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two) { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(one, two); \
    return exec(oper); \
}

#define CMD_3(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE)\
redis_protocol_t::redis_return_type CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two, ARG_TYPE_THREE three) { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(one, two, three); \
    return exec(oper); \
}

#define CMD_4(CNAME, ARG_TYPE_ONE, ARG_TYPE_TWO, ARG_TYPE_THREE, ARG_TYPE_FOUR)\
redis_protocol_t::redis_return_type CNAME(ARG_TYPE_ONE one, ARG_TYPE_TWO two, ARG_TYPE_THREE three, ARG_TYPE_FOUR four) { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(one, two, three, four); \
    return exec(oper); \
}

#define CMD_N(CNAME) \
redis_protocol_t::redis_return_type CNAME(std::vector<std::string> one) { \
    redis_protocol_t::CNAME *oper = new redis_protocol_t::CNAME(one); \
    return exec(oper); \
}

struct redis_ext {
    redis_ext(namespace_interface_t<redis_protocol_t> *intface) : namespace_interface(intface) {;}

    // KEYS
    CMD_N(del)
    CMD_1(exists, std::string&)
    CMD_2(expire, std::string&, unsigned)
    CMD_2(expireat, std::string&, unsigned)
    CMD_1(keys, std::string&)
    CMD_2(move, std::string&, std::string&)
    CMD_1(persist, std::string&)
    CMD_0(randomkey)

    //CMD_2(rename, std::string&, std::string&)
    redis_protocol_t::redis_return_type rename(std::string &key, std::string &newkey) {
        return rename_(key, newkey, false);
    }
    
    //CMD_2(renamenx, std::string&, std::string&)
    redis_protocol_t::redis_return_type renamenx(std::string &key, std::string &newkey) {
        return rename_(key, newkey, true);
    }

    CMD_1(ttl, std::string&)
    CMD_1(type, std::string&)

    //STRINGS
    CMD_2(append, std::string&, std::string&)
    CMD_1(decr, std::string&)
    CMD_2(decrby, std::string&, int)
    CMD_1(get, std::string&)
    CMD_2(getbit, std::string&, unsigned)
    CMD_3(getrange, std::string&, int, int)
    CMD_2(getset, std::string&, std::string&)
    CMD_1(incr, std::string&)
    CMD_2(incrby, std::string&, int)
    CMD_N(mget)
    CMD_N(mset)
    CMD_N(msetnx)

    //CMD_2(set, std::string&, std::string&)
    redis_protocol_t::redis_return_type set(std::string &key, std::string value) {
        on_thread_t(8);
        redis_protocol_t::set *oper = new redis_protocol_t::set(key, value);
        return exec(oper);
    }

    CMD_3(setbit, std::string&, unsigned, unsigned)
    CMD_3(setex, std::string&, unsigned, std::string&)
    CMD_2(setnx, std::string&, std::string&)
    CMD_3(setrange, std::string&, unsigned, std::string&)
    CMD_1(Strlen, std::string&)

    //Hashes
    CMD_N(hdel)
    CMD_2(hexists, std::string&, std::string&)
    CMD_2(hget, std::string&, std::string&)
    CMD_1(hgetall, std::string&)
    CMD_3(hincrby, std::string&, std::string&, int)
    CMD_1(hkeys, std::string&)
    CMD_1(hlen, std::string&)
    CMD_N(hmget)
    CMD_N(hmset)
    CMD_3(hset, std::string&, std::string&, std::string&)
    CMD_3(hsetnx, std::string&, std::string&, std::string&)
    CMD_1(hvals, std::string&)

    //Sets
    CMD_N(sadd)
    CMD_1(scard, std::string&)
    CMD_N(sdiff)

    //CMD_N(sdiffstore)
    redis_protocol_t::redis_return_type sdiffstore(std::vector<std::string> &destkeys) {
        std::string dest = destkeys[0];
        destkeys.erase(destkeys.begin());
        std::vector<std::string> result = boost::get<std::vector<std::string> >(sdiff(destkeys));
        int result_size = result.size();

        // This command overwrites dest, so we clear it first
        std::vector<std::string> vec;
        vec.push_back(dest);
        del(vec);

        // Then when we add the results we know we're not adding to an already existing set
        result.insert(result.begin(), dest);
        sadd(result);

        return result_size;
    }

    CMD_N(sinter)

    //CMD_N(sinterstore)
    redis_protocol_t::redis_return_type sinterstore(std::vector<std::string> &destkeys) {
        std::string dest = destkeys[0];
        destkeys.erase(destkeys.begin());
        std::vector<std::string> result = boost::get<std::vector<std::string> >(sinter(destkeys));
        int result_size = result.size();

        // This command overwrites dest, so we clear it first
        std::vector<std::string> vec;
        vec.push_back(dest);
        del(vec);

        // Then when we add the results we know we're not adding to an already existing set
        result.insert(result.begin(), dest);
        sadd(result);

        return result_size;
    }

    CMD_2(sismember, std::string&, std::string&)
    CMD_1(smembers, std::string&)
    
    //CMD_3(smove, std::string&, std::string&, std::string&)
    redis_protocol_t::redis_return_type smove(std::string &source, std::string &destination, std::string member) {
        std::vector<std::string> key_mem;
        key_mem.push_back(source);
        key_mem.push_back(member);
        int removed = boost::get<int>(srem(key_mem));
        if(removed != 1) {
            // Element doesn't exist in source
            return 0;
        }

        key_mem[0] = destination;
        sadd(key_mem);

        return 1;
    }

    CMD_1(spop, std::string&)
    CMD_1(srandmember, std::string&)
    CMD_N(srem)
    CMD_N(sunion)

    //CMD_N(sunionstore)
    redis_protocol_t::redis_return_type sunionstore(std::vector<std::string> &destkeys) {
        std::string dest = destkeys[0];
        destkeys.erase(destkeys.begin());
        std::vector<std::string> result = boost::get<std::vector<std::string> >(sunion(destkeys));
        int result_size = result.size();

        // This command overwrites dest, so we clear it first
        std::vector<std::string> vec;
        vec.push_back(dest);
        del(vec);

        // Then when we add the results we know we're not adding to an already existing set
        result.insert(result.begin(), dest);
        sadd(result);

        return result_size;
    }

    // Lists

    CMD_N(blpop)
    /*
    redis_protocol_t::redis_return_type blpop(std::vector<std::string> one) {
        // The last argument is the timeout
        std::string str_timeout = one.back();
        int timeout;
        try {
            timeout = boost::lexical_cast<int>(str_timeout);
        } catch(boost::bad_lexical_cast &) {
            throw "ERR Protocol Error";
        }

        // Try all the lists first with a normal pop
        for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end() - 1; ++iter) {
            redis_protocol_t::lpop *oper = new redis_protocol_t::lpop(*iter);
            redis_protocol_t::redis_return_type res = exec(oper);
            if(res.which() != 4) { // nil result
                // Found one
                return res;
            }
        }

        // Start a coroutine to do the blocking wait on each key
        wait_any_t pushed_on_a_list;
        std::string *list_with_pushed_value = NULL;

        for(std::vector<std::string>::iterator iter = one.begin(); iter != one.end() - 1; ++iter) {
            std::string *list = new std::string(*iter);
            cond_t *pushed = new cond_t();
            pushed_on_a_list.add(pushed);
            coro_t::spawn_later_ordered(boost::bind(wait_for_list_push, list, pushed, list_with_pushed_value));
        }

        // Set a timeout that will pulse pushed_on_a_list after a set timeout

        // Set this coroutine to wait for the timeout or to be woken by
        // a successful blocking pop.
        pushed_on_a_list.wait_lazily_ordered();

        // Cancel all other waits

        return redis_protocol_t::nil_result();
    }
    */

    CMD_N(brpop)
    CMD_N(brpoplpush)
    CMD_2(lindex, std::string&, int)
    CMD_4(linsert, std::string&, std::string&, std::string&, std::string&)
    CMD_1(llen, std::string&)
    CMD_1(lpop, std::string&)
    CMD_N(lpush)
    CMD_2(lpushx, std::string&, std::string&)
    CMD_3(lrange, std::string&, int, int)
    CMD_3(lrem, std::string&, int, std::string&)
    CMD_3(lset, std::string&, int, std::string&)
    CMD_3(ltrim, std::string&, int, int)
    CMD_1(rpop, std::string&)

    //CMD_2(rpoplpush, std::string&, std::string&)
    redis_protocol_t::redis_return_type rpoplpush(std::string &source, std::string &destination) {
        std::string str = boost::get<std::string>(rpop(source));
        std::vector<std::string> vec;
        vec.push_back(destination);
        vec.push_back(str);
        lpush(vec);
        return str;
    }

    CMD_N(rpush)
    CMD_2(rpushx, std::string&, std::string&)

    // Sorted Sets
    CMD_N(zadd)
    CMD_1(zcard, std::string)
    CMD_3(zcount, std::string, float, float)
    CMD_3(zincrby, std::string, float, std::string)
    CMD_N(zinterstore)

    redis_protocol_t::redis_return_type zrange(std::string &key, int start, int stop) {
        return exec(new redis_protocol_t::zrange(key, start, stop, false, false));
    }

    redis_protocol_t::redis_return_type zrange(std::string &key, int start, int stop, std::string &scores) {
        if(scores != std::string("WITHSCORES")) {
            return  redis_protocol_t::error_result("Protocol Error");
        }

        return exec(new redis_protocol_t::zrange(key, start, stop, true, false));
    }

    redis_protocol_t::redis_return_type zrangebyscore(std::string &key, std::string &score_min, std::string &score_max) {
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, false, false, 0, 0, false));
    }

    redis_protocol_t::redis_return_type zrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &with_scores) {
        if(with_scores != std::string("WITHSCORES")) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, true, false, 0, 0, false));
    }

    redis_protocol_t::redis_return_type zrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &limit, unsigned offset, unsigned count) {
        if(limit != std::string("LIMIT")) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, false, true, offset, count, false));
    }

    redis_protocol_t::redis_return_type zrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &with_scores, std::string &limit, unsigned offset, unsigned count) {
        if((with_scores != std::string("WITHSCORES")) || (limit != std::string("LIMIT"))) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, true, true, offset, count, false));
    }

    redis_protocol_t::redis_return_type zrank(std::string &key, std::string &member) {
        return exec(new redis_protocol_t::zrank(key, member, false));
    }

    CMD_N(zrem)
    CMD_3(zremrangebyrank, std::string, int, int)
    CMD_3(zremrangebyscore, std::string, float, float)

    redis_protocol_t::redis_return_type zrevrange(std::string &key, int start, int stop) {
        return exec(new redis_protocol_t::zrange(key, start, stop, false, true));
    }

    redis_protocol_t::redis_return_type zrevrange(std::string &key, int start, int stop, std::string &scores) {
        if(scores != std::string("WITHSCORES")) {
            return  redis_protocol_t::error_result("Protocol Error");
        }

        return exec(new redis_protocol_t::zrange(key, start, stop, true, true));
    }

    redis_protocol_t::redis_return_type zrevrangebyscore(std::string &key, std::string &score_min, std::string &score_max) {
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, false, false, 0, 0, true));
    }

    redis_protocol_t::redis_return_type zrevrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &with_scores) {
        if(with_scores != std::string("WITHSCORES")) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, true, false, 0, 0, true));
    }

    redis_protocol_t::redis_return_type zrevrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &limit, unsigned offset, unsigned count) {
        if(limit != std::string("LIMIT")) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, false, true, offset, count, true));
    }

    redis_protocol_t::redis_return_type zrevrangebyscore(std::string &key, std::string &score_min, std::string &score_max, std::string &with_scores, std::string &limit, unsigned offset, unsigned count) {
        if((with_scores != std::string("WITHSCORES")) || (limit != std::string("LIMIT"))) {
            return redis_protocol_t::error_result("Protocol Error");
        }
        return exec(new redis_protocol_t::zrangebyscore(key, score_min, score_max, true, true, offset, count, true));
    }

    redis_protocol_t::redis_return_type zrevrank(std::string &key, std::string &member) {
        return exec(new redis_protocol_t::zrank(key, member, true));
    }

    CMD_2(zscore, std::string, std::string)
    CMD_N(zunionstore)

    // Admin
    redis_protocol_t::redis_return_type select(unsigned index) {
        // TODO Someday this will select a namespace
        (void)index;
        return redis_protocol_t::status_result("OK");
    }

    redis_protocol_t::redis_return_type info() {
        // TODO real statistics
        return std::string("redis_version:2.2.10\r\nredis_git_sha1:00000000\r\nredis_git_dirty:0\r\narch_bits:64\r\nmultiplexing_api:epoll\r\nprocess_id:5969\r\nuptime_in_seconds:24\r\nuptime_in_days:0\r\nlru_clock:1693112\r\nused_cpu_sys:0.00\r\nused_cpu_user:0.00\r\nused_cpu_sys_childrens:0.00\r\nused_cpu_user_childrens:0.00\r\nconnected_clients:1\r\nconnected_slaves:0\r\nclient_longest_output_list:0\r\nclient_biggest_input_buf:0\r\nblocked_clients:0\r\nused_memory:800688\r\nused_memory_human:781.92K\r\nused_memory_rss:1675264\r\nmem_fragmentation_ratio:2.09\r\nuse_tcmalloc:0\r\nloading:0\r\naof_enabled:0\r\nchanges_since_last_save:0\r\nbgsave_in_progress:0\r\nlast_save_time:1317165345\r\nbgrewriteaof_in_progress:0\r\ntotal_connections_received:1\r\ntotal_commands_processed:0\r\nexpired_keys:0\r\nevicted_keys:0\r\nkeyspace_hits:0\r\nkeyspace_misses:0\r\nhash_max_zipmap_entries:512\r\nhash_max_zipmap_value:64\r\npubsub_channels:0\r\npubsub_patterns:0\r\nvm_enabled:0\r\nrole:master\r\nallocation_stats:2=1,6=1,8=1,9=5,10=10,11=7,12=15,13=40,14=27,15=35,16=10092,17=12,18=7,19=14,20=6,21=2,22=2,23=1,24=134,25=2,26=1,27=2,28=2,32=6,34=1,40=1,48=10,56=1,58=1,59=1,64=3,71=1,88=68,128=3,>=256=12\r\ndb0:keys=10,expires=0");
    }

private:
    namespace_interface_t<redis_protocol_t> *namespace_interface;
    
    redis_protocol_t::redis_return_type exec(redis_protocol_t::write_operation_t *oper) {
        redis_protocol_t::write_t write(oper);
        // TODO give interruptor
        cond_t signal;
        redis_protocol_t::write_response_t response = namespace_interface->write(write, order_token_t::ignore, &signal);
        if(response.get() == NULL) {
            return redis_protocol_t::nil_result();
        } else {
            return response->get_result();
        }
    }

    redis_protocol_t::redis_return_type rename_(std::string &key, std::string &newkey, bool nx) {
        if(key == newkey) {
            return redis_protocol_t::error_result("Protocol Error");
        }

        // We need to retrieve the old value. How we do this is actually depenedent upon the type of
        // the key. Setting the new value is also dependent on the type of key.
        redis_protocol_t::read_t oper(new redis_protocol_t::rename_get_type(key));
        // TODO give interruptor
        redis_protocol_t::read_response_t response = namespace_interface->read(oper, order_token_t::ignore, NULL);

        if(response.get() == NULL) {
            return redis_protocol_t::error_result("Key does not exist");
        }

        bool delete_ = true;

        // Get old value and set new value
        redis_value_type redis_type = static_cast<redis_value_type>(boost::get<int>(response->get_result()));
        std::string str;
        switch(redis_type) {
        case REDIS_STRING:
            str = boost::get<std::string>(exec(new redis_protocol_t::get(key)));
            if(nx) {
                delete_ = boost::get<int>(exec(new redis_protocol_t::setnx(newkey, str)));
            } else {
                exec(new redis_protocol_t::set(newkey, str));
            }
            break;
        // Renaming these other types is actually very hard
        case REDIS_LIST:
            // fallthrough
        case REDIS_HASH:
            // fallthrough
        case REDIS_SET:
            // fallthrough
        case REDIS_SORTED_SET:
            return redis_protocol_t::error_result("Cannot rename non-string keys");
        default:
            unreachable();
            break;
        }

        // And delete old key
        if(delete_) {
            std::vector<std::string> vec;
            vec.push_back(key);
            exec(new redis_protocol_t::del(vec));
        }

        return redis_protocol_t::status_result("OK");

    }

    redis_protocol_t::redis_return_type exec(redis_protocol_t::read_operation_t *oper) {
        redis_protocol_t::read_t read(oper);
        // TODO give interruptor
        cond_t signal;
        redis_protocol_t::read_response_t response = namespace_interface->read(read, order_token_t::ignore, &signal);
        if(response.get() == NULL) {
            return redis_protocol_t::nil_result();
        } else {
            return response->get_result();
        }
    }
};

#undef CMD_0
#undef CMD_1
#undef CMD_2
#undef CMD_3
#undef CMD_4
#undef CMD_N
