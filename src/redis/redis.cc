#include "redis/redis_types.hpp"
#include "redis/redis.hpp"
#include "btree/slice.hpp"
#include <iostream>
#include <string>
#include <vector>

struct keys_to_region : boost::static_visitor<redis_protocol_t::region_t> {
    redis_protocol_t::region_t operator()(std::vector<std::string> &keys) const {
        //TODO fix this 
        store_key_t s_key(keys[0]);
        redis_protocol_t::region_t r(key_range_t::open, s_key, key_range_t::closed, s_key);
        return r;
    }

    redis_protocol_t::region_t operator()(std::string key) const {
        store_key_t s_key(key);
        redis_protocol_t::region_t r(key_range_t::open, s_key, key_range_t::closed, s_key);
        return r;
    }
};

redis_protocol_t::region_t redis_protocol_t::read_t::get_region() {
    boost::variant<std::vector<std::string>, std::string> keys = op->get_keys();
    return boost::apply_visitor(keys_to_region(), keys);
}

redis_protocol_t::read_t redis_protocol_t::read_t::shard(region_t region) {
    return op->shard(region);
}

redis_protocol_t::read_response_t redis_protocol_t::read_t::unshard(const std::vector<redis_protocol_t::read_response_t> &responses, UNUSED redis_protocol_t::temporary_cache_t *cache) {
    read_response_t result(responses[0]);
    for(std::vector<read_response_t>::const_iterator iter = responses.begin() + 1; iter != responses.end(); ++iter) {
        result->deshard(iter->get());
    }
    return result;
}

redis_protocol_t::region_t redis_protocol_t::write_t::get_region() {
    boost::variant<std::vector<std::string>, std::string> keys = op->get_keys();
    return boost::apply_visitor(keys_to_region(), keys);
}

redis_protocol_t::write_t redis_protocol_t::write_t::shard(region_t region) {
    return op->shard(region);
}

redis_protocol_t::write_response_t redis_protocol_t::write_t::unshard(const std::vector<redis_protocol_t::write_response_t> &responses, UNUSED redis_protocol_t::temporary_cache_t *cache) {
    write_response_t result(responses[0]);
    for(std::vector<write_response_t>::const_iterator iter = responses.begin() + 1; iter != responses.end(); ++iter) {
        result->deshard(iter->get());
    }
    return result;
}

dummy_redis_store_view_t::dummy_redis_store_view_t(key_range_t region, btree_slice_t *b) :
    store_view_t<redis_protocol_t>(region), btree(b) { }

redis_protocol_t::read_response_t dummy_redis_store_view_t::do_read(const redis_protocol_t::read_t &r, UNUSED state_timestamp_t t, order_token_t otok, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    try {
        redis_protocol_t::read_response_t response = r.op->execute(btree, otok);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::read_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

redis_protocol_t::write_response_t dummy_redis_store_view_t::do_write(const redis_protocol_t::write_t &w, transition_timestamp_t t, order_token_t otok, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    try {
        redis_protocol_t::write_response_t response = w.op->execute(btree, t, otok);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::write_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

// Individual commands are implemented in their respective files: keys, strings, etc.
