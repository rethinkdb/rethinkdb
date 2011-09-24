#ifndef __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__
#define __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__

#include <map>
#include <set>
#include <string>
#include <vector>

#include "utils.hpp"
#include <boost/function.hpp>

#include "concurrency/fifo_checker.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

struct signal_t;

namespace unittest {

class dummy_protocol_t {

public:

    class region_t {

    public:
        static region_t empty() THROWS_NOTHING;

        std::set<std::string> keys;
    };

    class temporary_cache_t {
        /* Dummy protocol doesn't need to cache anything */
    };

    class read_response_t {

    public:
        std::map<std::string, std::string> values;
    };

    class read_t {

    public:
        region_t get_region() const;
        read_t shard(region_t region) const;
        read_response_t unshard(std::vector<read_response_t> resps, temporary_cache_t *cache) const;

        region_t keys;
    };

    class write_response_t {

    public:
        std::map<std::string, std::string> old_values;
    };

    class write_t {

    public:
        region_t get_region() const;
        write_t shard(region_t region) const;
        write_response_t unshard(std::vector<write_response_t> resps, temporary_cache_t *cache) const;

        std::map<std::string, std::string> values;
    };

    class backfill_chunk_t {

    public:
        std::string key, value;
        state_timestamp_t timestamp;
    };
};

bool region_is_superset(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
dummy_protocol_t::region_t region_intersection(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
dummy_protocol_t::region_t region_join(std::vector<dummy_protocol_t::region_t> vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t);

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);

class dummy_underlying_store_t {

public:
    dummy_underlying_store_t(dummy_protocol_t::region_t);

    dummy_protocol_t::region_t region;
    std::map<std::string, std::string> values;
    std::map<std::string, state_timestamp_t> timestamps;

    order_sink_t order_sink;
};

class dummy_store_view_t : public store_view_t<dummy_protocol_t> {

public:
    dummy_store_view_t(dummy_underlying_store_t *parent, dummy_protocol_t::region_t region);

protected:
    dummy_protocol_t::read_response_t do_read(const dummy_protocol_t::read_t &read, state_timestamp_t timestamp, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);
    dummy_protocol_t::write_response_t do_write(const dummy_protocol_t::write_t &write, transition_timestamp_t timestamp, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t);

    void do_send_backfill(
        std::vector<std::pair<dummy_protocol_t::region_t, state_timestamp_t> > start_point,
        boost::function<void(dummy_protocol_t::backfill_chunk_t)> chunk_sender,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

    void do_receive_backfill(
        dummy_protocol_t::backfill_chunk_t chunk,
        signal_t *interruptor)
        THROWS_ONLY(interrupted_exc_t);

private:
    dummy_underlying_store_t *parent;

    /* For random timeouts */
    rng_t rng;
};

}   /* namespace unittest */

#endif /* __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__ */
