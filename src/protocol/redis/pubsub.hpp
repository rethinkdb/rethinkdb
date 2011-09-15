#ifndef __PROTOCOL_REDIS_PUBSUB__HPP__
#define __PROTOCOL_REDIS_PUBSUB__HPP__

#include "concurrency/rwi_lock.hpp"
#include "arch/arch.hpp"
#include "protocol/redis/redis_proto.hpp"
#include <map>
#include <list>

struct subscription_t {
    uint64_t connection_id;
    redis_output_writer *out;
};

struct pattern_subscription_t : subscription_t {
    std::string pattern;
};

struct pubsub_runtime_t {
    bool subscribe(std::string &channel, uint64_t connection_id, redis_output_writer *out);
    bool subscribe_pattern(std::string &pattern, uint64_t connection_id, redis_output_writer *out);
    bool unsubscribe(std::string &channel, uint64_t connection_id);
    bool unsubscribe_pattern(std::string &pattern, uint64_t connection_id);
    int publish(std::string &channel, std::string &message);

private:
    static bool pattern_matches(std::string &channel, std::string &pattern);

    rwi_lock_t subs_lock;
    std::map<std::string, std::list<subscription_t> > subscriptions;
    std::list<pattern_subscription_t> pattern_subscriptions;
};

#endif /*__PROTOCOL_REDIS_PUBSUB__HPP__*/
