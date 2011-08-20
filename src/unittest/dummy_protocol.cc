#include "unittest/dummy_protocol.hpp"
#include "concurrency/signal.hpp"

namespace unittest {

bool dummy_protocol_t::region_t::contains(const region_t &r) {
    for (std::set<std::string>::iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) == 0) {
            return false;
        }
    }
    return true;
}

bool dummy_protocol_t::region_t::overlaps(const region_t &r) {
    for (std::set<std::string>::iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) != 0) {
            return true;
        }
    }
    return false;
}

dummy_protocol_t::region_t dummy_protocol_t::region_t::intersection(const region_t &r) {
    region_t i;
    for (std::set<std::string>::iterator it = r.keys.begin(); it != r.keys.end(); it++) {
        if (keys.count(*it) != 0) {
            i.keys.insert(*it);
        }
    }
    return i;
}

dummy_protocol_t::region_t dummy_protocol_t::read_t::get_region() {
    return keys;
}

std::vector<dummy_protocol_t::read_t> dummy_protocol_t::read_t::shard(std::vector<region_t> regions) {
    std::vector<read_t> results;
    for (int i = 0; i < (int)regions.size(); i++) {
        read_t r;
        r.keys = regions[i].intersection(keys);
        results.push_back(r);
    }
    return results;
}

std::vector<dummy_protocol_t::read_t> dummy_protocol_t::read_t::parallelize(UNUSED int optimal_factor) {
    std::vector<read_t> vec;
    for (std::set<std::string>::iterator it = keys.keys.begin();
            it != keys.keys.end(); it++) {
        read_t subread;
        subread.keys.keys.insert(*it);
        vec.push_back(subread);
    }
    return vec;
}

dummy_protocol_t::read_response_t dummy_protocol_t::read_response_t::unshard(std::vector<read_response_t> resps) {
    read_response_t combined;
    for (int i = 0; i < (int)resps.size(); i++) {
        for (std::map<std::string, std::string>::iterator it = resps[i].values.begin();
                it != resps[i].values.end(); it++) {
            combined.values[(*it).first] = (*it).second;
        }
    }
    return combined;
}

dummy_protocol_t::read_response_t dummy_protocol_t::read_response_t::unparallelize(std::vector<read_response_t> resps) {
    read_response_t combined;
    for (int i = 0; i < (int)resps.size(); i++) {
        for (std::map<std::string, std::string>::iterator it = resps[i].values.begin();
                it != resps[i].values.end(); it++) {
            combined.values[(*it).first] = (*it).second;
        }
    }
    return combined;
}

dummy_protocol_t::region_t dummy_protocol_t::write_t::get_region() {
    region_t region;
    for (std::map<std::string, std::string>::iterator it = values.begin();
            it != values.end(); it++) {
        region.keys.insert((*it).first);
    }
    return region;
}

std::vector<dummy_protocol_t::write_t> dummy_protocol_t::write_t::shard(std::vector<region_t> regions) {
    std::vector<write_t> results;
    for (int i = 0; i < (int)regions.size(); i++) {
        write_t w;
        for (std::map<std::string, std::string>::iterator it = values.begin();
                it != values.end(); it++) {
            if (regions[i].keys.count((*it).first) != 0) {
                w.values[(*it).first] = (*it).second;
            }
        }
        results.push_back(w);
    }
    return results;
}

dummy_protocol_t::write_response_t dummy_protocol_t::write_response_t::unshard(std::vector<write_response_t> resps) {
    write_response_t combined;
    for (int i = 0; i < (int)resps.size(); i++) {
        for (std::map<std::string, std::string>::iterator it = resps[i].old_values.begin();
                it != resps[i].old_values.end(); it++) {
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

repli_timestamp_t dummy_protocol_t::store_t::get_timestamp() {
    rassert(!backfilling);
    return latest;
}

dummy_protocol_t::read_response_t dummy_protocol_t::store_t::read(read_t read, order_token_t otok) {

    rassert(!backfilling);
    rassert(coherent);
    order_sink.check_out(otok);

    read_response_t resp;
    for (std::set<std::string>::iterator it = read.keys.keys.begin();
            it != read.keys.keys.end(); it++) {
        rassert(region.keys.count(*it) != 0);
        resp.values[*it] = values[*it];
    }
    return resp;
}

dummy_protocol_t::write_response_t dummy_protocol_t::store_t::write(write_t write, repli_timestamp_t timestamp, order_token_t otok) {

    rassert(!backfilling);
    rassert(coherent);
    rassert(timestamp >= latest);
    rassert(earliest == latest);
    order_sink.check_out(otok);

    earliest = latest = timestamp;

    write_response_t resp;
    for (std::map<std::string, std::string>::iterator it = write.values.begin();
            it != write.values.end(); it++) {
        resp.old_values[(*it).first] = values[(*it).first];
        values[(*it).first] = (*it).second;
        timestamps[(*it).first] = timestamp;
    }
    return resp;
}

bool dummy_protocol_t::store_t::is_backfilling() {
    return backfilling;
}

dummy_protocol_t::store_t::backfill_request_t dummy_protocol_t::store_t::backfillee_begin() {
    rassert(!backfilling);
    backfilling = true;
    backfill_request_t bfr;
    bfr.region = region;
    bfr.earliest = earliest;
    bfr.latest = latest;
    return bfr;
}

void dummy_protocol_t::store_t::backfillee_chunk(backfill_chunk_t chunk) {
    rassert(backfilling);
    rassert(region.keys.count(chunk.key) != 0);
    values[chunk.key] = chunk.value;
    timestamps[chunk.key] = chunk.timestamp;
    latest = chunk.timestamp;
}

void dummy_protocol_t::store_t::backfillee_end(backfill_end_t end) {
    rassert(backfilling);
    coherent = true;
    earliest = latest = end.timestamp;
    backfilling = false;
}

void dummy_protocol_t::store_t::backfillee_cancel() {
    rassert(backfilling);
    coherent = false;
    backfilling = false;
}

dummy_protocol_t::store_t::backfill_end_t dummy_protocol_t::store_t::backfiller(backfill_request_t request,
    boost::function<void(backfill_chunk_t)> chunk_fun,
    signal_t *interruptor)
{
    rassert(!backfilling);
    rassert(coherent);
    rassert(request.region == region);
    rassert(request.latest <= latest);

    if (interruptor->is_pulsed()) throw interrupted_exc_t();

    for (std::map<std::string, std::string>::iterator it = values.begin();
            it != values.end(); it++) {
        if (timestamps[(*it).first] >= request.earliest) {
            backfill_chunk_t chunk;
            chunk.key = (*it).first;
            chunk.value = (*it).second;
            chunk.timestamp = timestamps[(*it).first];
            chunk_fun(chunk);
        }
    }

    backfill_end_t end;
    end.timestamp = latest;
    return end;
}

dummy_protocol_t::store_t::store_t(region_t r) : region(r), coherent(true), backfilling(false),
    earliest(repli_timestamp_t::distant_past), latest(earliest)
{
    for (std::set<std::string>::iterator it = region.keys.begin();
            it != region.keys.end(); it++) {
        values[*it] = "";
        timestamps[*it] = repli_timestamp_t::distant_past;
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
