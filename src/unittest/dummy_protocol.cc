#include "unittest/dummy_protocol.hpp"

#include "arch/timing.hpp"
#include "concurrency/signal.hpp"

namespace unittest {

bool dummy_protocol_t::region_t::contains(const region_t &r) const {
    for (std::set<std::string>::const_iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) == 0) {
            return false;
        }
    }
    return true;
}

bool dummy_protocol_t::region_t::overlaps(const region_t &r) const {
    for (std::set<std::string>::const_iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) != 0) {
            return true;
        }
    }
    return false;
}

dummy_protocol_t::region_t dummy_protocol_t::region_t::intersection(const region_t &r) const {
    region_t i;
    for (std::set<std::string>::const_iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) != 0) {
            i.keys.insert(*it);
        }
    }
    return i;
}

bool dummy_protocol_t::region_t::covered_by(std::vector<region_t> regions) const {
    for (std::vector<region_t>::iterator it = regions.begin(); it != regions.end(); it++) {
        for (std::set<std::string>::iterator it2 = (*it).keys.begin(); it2 != (*it).keys.end(); it2++) {
            if (keys.count(*it2) == 0) return false;
        }
    }
    return true;
}

dummy_protocol_t::region_t dummy_protocol_t::read_t::get_region() const {
    return keys;
}

std::vector<dummy_protocol_t::read_t> dummy_protocol_t::read_t::shard(std::vector<region_t> regions) const {
    std::vector<read_t> results;
    for (int i = 0; i < (int)regions.size(); i++) {
        read_t r;
        r.keys = regions[i].intersection(keys);
        results.push_back(r);
    }
    return results;
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

std::vector<dummy_protocol_t::write_t> dummy_protocol_t::write_t::shard(std::vector<region_t> regions) const {
    std::vector<write_t> results;
    for (int i = 0; i < (int)regions.size(); i++) {
        write_t w;
        for (std::map<std::string, std::string>::const_iterator it = values.begin();
                it != values.end(); it++) {
            if (regions[i].keys.count((*it).first) != 0) {
                w.values[(*it).first] = (*it).second;
            }
        }
        results.push_back(w);
    }
    return results;
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

dummy_protocol_t::region_t dummy_protocol_t::store_t::get_region() {
    return region;
}

bool dummy_protocol_t::store_t::is_coherent() {
    rassert(!backfilling);
    return coherent;
}

state_timestamp_t dummy_protocol_t::store_t::get_timestamp() {
    rassert(!backfilling);
    return latest_timestamp;
}

dummy_protocol_t::read_response_t dummy_protocol_t::store_t::read(read_t read, order_token_t otok, signal_t *interruptor) {
    rassert(!backfilling);
    rassert(coherent);
    order_sink.check_out(otok);

    if (interruptor->is_pulsed() && rng.randint(2)) throw interrupted_exc_t();

    read_response_t resp;
    for (std::set<std::string>::iterator it = read.keys.keys.begin();
            it != read.keys.keys.end(); it++) {
        rassert(region.keys.count(*it) != 0);
        resp.values[*it] = values[*it];
    }

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

    return resp;
}

dummy_protocol_t::write_response_t dummy_protocol_t::store_t::write(write_t write, transition_timestamp_t timestamp, order_token_t otok, signal_t *interruptor) {
    rassert(!backfilling);
    rassert(coherent);
    rassert(earliest_timestamp == latest_timestamp);
    rassert(timestamp.timestamp_before() == latest_timestamp);
    order_sink.check_out(otok);

    if (interruptor->is_pulsed() && rng.randint(2)) throw interrupted_exc_t();

    earliest_timestamp = latest_timestamp = timestamp.timestamp_after();

    write_response_t resp;
    for (std::map<std::string, std::string>::iterator it = write.values.begin();
            it != write.values.end(); it++) {
        resp.old_values[(*it).first] = values[(*it).first];
        values[(*it).first] = (*it).second;
        timestamps[(*it).first] = timestamp.timestamp_after();
    }

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

    return resp;
}

bool dummy_protocol_t::store_t::is_backfilling() {
    return backfilling;
}

dummy_protocol_t::region_t dummy_protocol_t::store_t::backfill_request_t::get_region() const {
    return region;
}

state_timestamp_t dummy_protocol_t::store_t::backfill_request_t::get_timestamp() const {
    return latest_timestamp;
}

dummy_protocol_t::store_t::backfill_request_t dummy_protocol_t::store_t::backfillee_begin() {
    rassert(!backfilling);
    backfilling = true;
    backfill_request_t bfr;
    bfr.region = region;
    bfr.earliest_timestamp = earliest_timestamp;
    bfr.latest_timestamp = latest_timestamp;
    return bfr;
}

void dummy_protocol_t::store_t::backfillee_chunk(backfill_chunk_t chunk) {
    rassert(backfilling);
    rassert(region.keys.count(chunk.key) != 0);
    values[chunk.key] = chunk.value;
    timestamps[chunk.key] = chunk.timestamp;
    if (chunk.timestamp > latest_timestamp) latest_timestamp = chunk.timestamp;
}

void dummy_protocol_t::store_t::backfillee_end(state_timestamp_t end) {
    rassert(backfilling);
    coherent = true;
    earliest_timestamp = latest_timestamp = end;
    backfilling = false;
}

void dummy_protocol_t::store_t::backfillee_cancel() {
    rassert(backfilling);
    coherent = false;
    backfilling = false;
}

state_timestamp_t dummy_protocol_t::store_t::backfiller(backfill_request_t request,
    boost::function<void(backfill_chunk_t)> chunk_fun,
    signal_t *interruptor)
{
    rassert(!backfilling);
    rassert(coherent);
    rassert(request.region == region);
    rassert(request.latest_timestamp <= latest_timestamp);
    rassert(latest_timestamp == earliest_timestamp);

    /* Make a copy so we can sleep and still have the correct semantics */
    std::map<std::string, std::string> values_snapshot = values;
    std::map<std::string, state_timestamp_t> timestamps_snapshot = timestamps;
    state_timestamp_t latest_timestamp_snapshot = latest_timestamp;

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

    for (std::map<std::string, std::string>::iterator it = values_snapshot.begin();
            it != values_snapshot.end(); it++) {
        if (timestamps_snapshot[(*it).first] >= request.earliest_timestamp) {
            backfill_chunk_t chunk;
            chunk.key = (*it).first;
            chunk.value = (*it).second;
            chunk.timestamp = timestamps_snapshot[(*it).first];
            chunk_fun(chunk);
        }
        if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
    }

    return latest_timestamp_snapshot;
}

dummy_protocol_t::store_t::store_t(region_t r) : region(r), coherent(true), backfilling(false),
    earliest_timestamp(state_timestamp_t::zero()), latest_timestamp(earliest_timestamp)
{
    for (std::set<std::string>::iterator it = region.keys.begin();
            it != region.keys.end(); it++) {
        values[*it] = "";
        timestamps[*it] = state_timestamp_t::zero();
    }
}

dummy_protocol_t::store_t::~store_t() {
    rassert(!backfilling);
}

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return a.keys == b.keys;
}

bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return !(a == b);
}

}   /* namespace unittest */
