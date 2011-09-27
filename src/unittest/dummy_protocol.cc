#include "unittest/dummy_protocol.hpp"

#include "errors.hpp"
#include <boost/make_shared.hpp>

#include "arch/timing.hpp"
#include "concurrency/signal.hpp"

namespace unittest {

dummy_protocol_t::region_t dummy_protocol_t::region_t::empty() THROWS_NOTHING {
    return region_t();
}

dummy_protocol_t::region_t dummy_protocol_t::read_t::get_region() const {
    return keys;
}

dummy_protocol_t::read_t dummy_protocol_t::read_t::shard(region_t region) const {
    rassert(region_is_superset(get_region(), region));
    read_t r;
    r.keys = region_intersection(region, keys);
    return r;
}

dummy_protocol_t::read_response_t dummy_protocol_t::read_t::unshard(std::vector<read_response_t> resps, UNUSED temporary_cache_t *cache) const {
    rassert(cache != NULL);
    read_response_t combined;
    for (int i = 0; i < (int)resps.size(); i++) {
        for (std::map<std::string, std::string>::const_iterator it = resps[i].values.begin();
                it != resps[i].values.end(); it++) {
            rassert(keys.keys.count((*it).first) != 0);
            rassert(combined.values.count((*it).first) == 0);
            combined.values[(*it).first] = (*it).second;
        }
    }
    return combined;
}

dummy_protocol_t::region_t dummy_protocol_t::write_t::get_region() const {
    region_t region;
    for (std::map<std::string, std::string>::const_iterator it = values.begin();
            it != values.end(); it++) {
        region.keys.insert((*it).first);
    }
    return region;
}

dummy_protocol_t::write_t dummy_protocol_t::write_t::shard(region_t region) const {
    rassert(region_is_superset(get_region(), region));
    write_t w;
    for (std::map<std::string, std::string>::const_iterator it = values.begin();
            it != values.end(); it++) {
        if (region.keys.count((*it).first) != 0) {
            w.values[(*it).first] = (*it).second;
        }
    }
    return w;
}

dummy_protocol_t::write_response_t dummy_protocol_t::write_t::unshard(std::vector<write_response_t> resps, UNUSED temporary_cache_t *cache) const {
    rassert(cache != NULL);
    write_response_t combined;
    for (int i = 0; i < (int)resps.size(); i++) {
        for (std::map<std::string, std::string>::const_iterator it = resps[i].old_values.begin();
                it != resps[i].old_values.end(); it++) {
            rassert(values.find((*it).first) != values.end());
            rassert(combined.old_values.count((*it).first) == 0);
            combined.old_values[(*it).first] = (*it).second;
        }
    }
    return combined;
}

bool region_is_superset(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    for (std::set<std::string>::const_iterator it = b.keys.begin(); it != b.keys.end(); it++) {
        if (a.keys.count(*it) == 0) {
            return false;
        }
    }
    return true;
}

dummy_protocol_t::region_t region_intersection(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    dummy_protocol_t::region_t i;
    for (std::set<std::string>::const_iterator it = a.keys.begin(); it != a.keys.end(); it++) {
        if (b.keys.count(*it) != 0) {
            i.keys.insert(*it);
        }
    }
    return i;
}

dummy_protocol_t::region_t region_join(std::vector<dummy_protocol_t::region_t> vec) THROWS_ONLY(bad_join_exc_t, bad_region_exc_t) {
    dummy_protocol_t::region_t u;
    for (std::vector<dummy_protocol_t::region_t>::iterator it = vec.begin(); it != vec.end(); it++) {
        for (std::set<std::string>::iterator it2 = (*it).keys.begin(); it2 != (*it).keys.end(); it2++) {
            if (u.keys.count(*it2) != 0) throw bad_join_exc_t();
            u.keys.insert(*it2);
        }
    }
    return u;
}

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return a.keys == b.keys;
}

bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return !(a == b);
}

dummy_underlying_store_t::dummy_underlying_store_t(dummy_protocol_t::region_t r) :
        region(r)
{
    for (std::set<std::string>::iterator it = region.keys.begin();
            it != region.keys.end(); it++) {
        values[*it] = "";
        timestamps[*it] = state_timestamp_t::zero();
    }
}

dummy_store_view_t::dummy_store_view_t(dummy_underlying_store_t *p, dummy_protocol_t::region_t region) :
    store_view_t<dummy_protocol_t>(region), parent(p)
{
    rassert(region_is_superset(parent->region, region));
}

dummy_protocol_t::read_response_t dummy_store_view_t::do_read(const dummy_protocol_t::read_t &read, state_timestamp_t timestamp, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    parent->order_sink.check_out(otok);

    if (interruptor->is_pulsed() && rng.randint(2)) throw interrupted_exc_t();

    dummy_protocol_t::read_response_t resp;
    for (std::set<std::string>::iterator it = read.keys.keys.begin();
            it != read.keys.keys.end(); it++) {
        rassert(get_region().keys.count(*it) != 0);
        resp.values[*it] = parent->values[*it];
        rassert(parent->timestamps[*it] <= timestamp);
    }

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

    return resp;
}

dummy_protocol_t::write_response_t dummy_store_view_t::do_write(const dummy_protocol_t::write_t &write, UNUSED transition_timestamp_t timestamp, order_token_t otok) THROWS_NOTHING {
    parent->order_sink.check_out(otok);

    dummy_protocol_t::write_response_t resp;
    for (std::map<std::string, std::string>::const_iterator it = write.values.begin();
            it != write.values.end(); it++) {
        resp.old_values[(*it).first] = parent->values[(*it).first];
        parent->values[(*it).first] = (*it).second;
        parent->timestamps[(*it).first] = timestamp.timestamp_after();
    }

    return resp;
}

void dummy_store_view_t::do_send_backfill(
    std::vector<std::pair<dummy_protocol_t::region_t, state_timestamp_t> > start_point,
    boost::function<void(dummy_protocol_t::backfill_chunk_t)> chunk_sender,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t)
{
    /* Make a copy so we can sleep and still have the correct semantics */
    std::map<std::string, std::string> values_snapshot = parent->values;
    std::map<std::string, state_timestamp_t> timestamps_snapshot = parent->timestamps;

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

    for (int i = 0; i < (int)start_point.size(); i++) {
        for (std::set<std::string>::iterator it = start_point[i].first.keys.begin();
                it != start_point[i].first.keys.end(); it++) {
            if (timestamps_snapshot[*it] > start_point[i].second) {
                dummy_protocol_t::backfill_chunk_t chunk;
                chunk.key = *it;
                chunk.value = values_snapshot[*it];
                chunk.timestamp = timestamps_snapshot[*it];
                chunk_sender(chunk);
            }
            if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        }
    }
}

void dummy_store_view_t::do_receive_backfill(
    dummy_protocol_t::backfill_chunk_t chunk,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t)
{
    rassert(get_region().keys.count(chunk.key) != 0);
    if (interruptor->is_pulsed() && rng.randint(2)) throw interrupted_exc_t();
    parent->values[chunk.key] = chunk.value;
    parent->timestamps[chunk.key] = chunk.timestamp;
    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
}

}   /* namespace unittest */
