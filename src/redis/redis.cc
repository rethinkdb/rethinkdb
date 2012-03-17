#include "redis/redis_types.hpp"
#include "redis/redis.hpp"
#include "btree/slice.hpp"
#include "errors.hpp"
#include <string>
#include <vector>

struct keys_to_region : boost::static_visitor<redis_protocol_t::region_t> {
    redis_protocol_t::region_t operator()(std::vector<std::string> &keys) const {
        //TODO fix this 
        store_key_t s_key(keys[0]);
        redis_protocol_t::region_t r(key_range_t::closed, s_key, key_range_t::closed, s_key);
        return r;
    }

    redis_protocol_t::region_t operator()(const std::string& key) const {
        store_key_t s_key(key);
        redis_protocol_t::region_t r(key_range_t::closed, s_key, key_range_t::closed, s_key);
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

/*
dummy_redis_store_view_t::dummy_redis_store_view_t(key_range_t region, btree_slice_t *b) :
        store_view_t<redis_protocol_t>(region), btree(b) {
    binary_blob_t empty;
    region_map_t<redis_protocol_t, binary_blob_t> map(region, empty);
    metadata = map;
}

boost::shared_ptr<store_view_t<redis_protocol_t>::read_transaction_t>
        dummy_redis_store_view_t::begin_read_transaction(
                UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    return boost::shared_ptr<dummy_redis_store_view_t::transaction_t>(
                new dummy_redis_store_view_t::transaction_t(this)
           );
}

boost::shared_ptr<store_view_t<redis_protocol_t>::write_transaction_t>
        dummy_redis_store_view_t::begin_write_transaction(
                UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    return boost::shared_ptr<dummy_redis_store_view_t::transaction_t>(
                new dummy_redis_store_view_t::transaction_t(this)
           );
}

dummy_redis_store_view_t::transaction_t::transaction_t(dummy_redis_store_view_t *parent_) : parent(parent_) {}

region_map_t<redis_protocol_t, binary_blob_t> dummy_redis_store_view_t::transaction_t::get_metadata(
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)interruptor;
    return parent->metadata;
}

redis_protocol_t::read_response_t dummy_redis_store_view_t::transaction_t::read(
        const redis_protocol_t::read_t &read,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)interruptor;
    try {
        redis_protocol_t::read_response_t response = read.op->execute(parent->btree, order_token_t::ignore);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::read_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

void dummy_redis_store_view_t::transaction_t::send_backfill(
        const region_map_t<redis_protocol_t, state_timestamp_t> &start_point,
        const boost::function<void(redis_protocol_t::backfill_chunk_t)> &chunk_fun,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)start_point;
    (void)chunk_fun;
    (void)interruptor;
}

void dummy_redis_store_view_t::transaction_t::set_metadata(
        const region_map_t<redis_protocol_t, binary_blob_t> &new_metadata) THROWS_NOTHING {
    parent->metadata = new_metadata;
}

redis_protocol_t::write_response_t dummy_redis_store_view_t::transaction_t::write(
        const redis_protocol_t::write_t &write,
        transition_timestamp_t timestamp)
        THROWS_NOTHING {
    try {
        redis_protocol_t::write_response_t response = write.op->execute(parent->btree, timestamp, order_token_t::ignore);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::write_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

void dummy_redis_store_view_t::transaction_t::receive_backfill(
        const redis_protocol_t::backfill_chunk_t &chunk_fun,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t) {
    (void)chunk_fun;
    (void)interruptor;
}
*/
/* OUTDATED

redis_protocol_t::read_response_t dummy_redis_store_view_t::do_read(const redis_protocol_t::read_t &r, UNUSED state_timestamp_t t, order_token_t otok, UNUSED signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    try {
        redis_protocol_t::read_response_t response = r.op->execute(btree, otok);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::read_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

redis_protocol_t::write_response_t dummy_redis_store_view_t::do_write(const redis_protocol_t::write_t &w, transition_timestamp_t t, order_token_t otok) THROWS_NOTHING {
    try {
        redis_protocol_t::write_response_t response = w.op->execute(btree, t, otok);
        return response;
    } catch(const char *msg) {
        return redis_protocol_t::write_response_t(new redis_protocol_t::error_result_t(msg));
    }
}

*/

// Individual commands are implemented in their respective files: keys, strings, etc.
