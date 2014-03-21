// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "mock/dummy_protocol.hpp"

// TODO: Move version_range_t out of clustering/immediate_consistency/branch/metadata.hpp.
#include "arch/timing.hpp"
#include "btree/btree_store.hpp"
#include "clustering/immediate_consistency/branch/metadata.hpp"
#include "concurrency/signal.hpp"
#include "concurrency/wait_any.hpp"
#include "containers/printf_buffer.hpp"
#include "debug.hpp"
#include "mock/serializer_filestream.hpp"

namespace mock {

dummy_protocol_t::region_t dummy_protocol_t::region_t::empty() THROWS_NOTHING {
    return region_t();
}

dummy_protocol_t::region_t dummy_protocol_t::region_t::universe() THROWS_NOTHING {
    return a_thru_z_region();
}

bool dummy_protocol_t::region_t::operator<(const region_t &other) const {
    std::set<std::string>::iterator it_us = keys.begin();
    std::set<std::string>::iterator it_other = other.keys.begin();

    while (true) {
        if (it_us == keys.end() && it_other == other.keys.end()) {
            return false;
        } else if (it_us == keys.end()) {
            return true;
        } else if (it_other == other.keys.end()) {
            return false;
        } else if (*it_us == *it_other) {
            it_us++;
            it_other++;
            continue;
        } else {
            return *it_us < *it_other;
        }
    }
}

dummy_protocol_t::region_t::region_t() THROWS_NOTHING {
}

dummy_protocol_t::region_t::region_t(char x, char y) THROWS_NOTHING {
    rassert(y >= x);
    for (char c = x; c <= y; c++) {
        keys.insert(std::string(1, c));
    }
}

dummy_protocol_t::region_t dummy_protocol_t::read_t::get_region() const {
    return keys;
}

bool dummy_protocol_t::read_t::shard(const region_t &region,
                                     read_t *read_out) const {
    region_t intersection = region_intersection(region, keys);
    if (!region_is_empty(intersection)) {
        *read_out = read_t();
        read_out->keys = intersection;
        return true;
    } else {
        return false;
    }
}

void dummy_protocol_t::read_t::unshard(const read_response_t *resps, size_t
        count, dummy_protocol_t::read_response_t *response,
        DEBUG_VAR context_t *ctx, signal_t *) const {
    rassert(ctx != NULL);
    for (size_t i = 0; i < count; ++i) {
        for (std::map<std::string, std::string>::const_iterator it = resps[i].values.begin();
                it != resps[i].values.end(); it++) {
            rassert(keys.keys.count(it->first) != 0,
                "We got a response that doesn't match our request");
            rassert(response->values.count(it->first) == 0,
                "Part of the query was run multiple times, or a response was "
                "duplicated.");
            response->values[it->first] = it->second;
        }
    }
}

dummy_protocol_t::region_t dummy_protocol_t::write_t::get_region() const {
    region_t region;
    for (std::map<std::string, std::string>::const_iterator it = values.begin();
            it != values.end(); it++) {
        region.keys.insert(it->first);
    }
    return region;
}

bool dummy_protocol_t::write_t::shard(const region_t &region,
                                      write_t *write_out) const {
    std::map<std::string, std::string> tmp;
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (region.keys.find(it->first) != region.keys.end()) {
            tmp.insert(*it);
        }
    }
    if (!tmp.empty()) {
        *write_out = write_t();
        write_out->values.swap(tmp);
        return true;
    } else {
        return false;
    }
}

void dummy_protocol_t::write_t::unshard(const write_response_t* resps, size_t count, write_response_t *response, DEBUG_VAR context_t *ctx, signal_t *) const {
    rassert(ctx != NULL);
    for (size_t i = 0; i < count; ++i) {
        for (std::map<std::string, std::string>::const_iterator it = resps[i].old_values.begin();
                it != resps[i].old_values.end(); it++) {
            rassert(values.find(it->first) != values.end(),
                "We got a response that doesn't match our request.");
            rassert(response->old_values.count(it->first) == 0,
                "Part of the query was run multiple times, or a response was "
                "duplicated.");
            response->old_values[it->first] = it->second;
        }
    }
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

region_join_result_t region_join(const std::vector<dummy_protocol_t::region_t>& vec, dummy_protocol_t::region_t *out) THROWS_NOTHING {
    dummy_protocol_t::region_t u;
    for (std::vector<dummy_protocol_t::region_t>::const_iterator it = vec.begin(); it != vec.end(); it++) {
        for (std::set<std::string>::iterator it2 = it->keys.begin(); it2 != it->keys.end(); it2++) {
            if (u.keys.count(*it2) != 0) {
                return REGION_JOIN_BAD_JOIN;
            }
            u.keys.insert(*it2);
        }
    }
    *out = u;
    return REGION_JOIN_OK;
}

std::vector<dummy_protocol_t::region_t> region_subtract_many(const dummy_protocol_t::region_t &a, const std::vector<dummy_protocol_t::region_t>& b) {
    std::vector<dummy_protocol_t::region_t> result(1, a);

    for (size_t i = 0; i < b.size(); i++) {
        for (std::set<std::string>::const_iterator j = b[i].keys.begin(); j != b[i].keys.end(); ++j) {
            result[0].keys.erase(*j);
        }
    }
    if (region_is_empty(result[0])) {
        result.clear();
    }
    return result;
}


bool region_is_empty(const dummy_protocol_t::region_t &r) {
    return r.keys.empty();
}


bool region_overlaps(const dummy_protocol_t::region_t &r1, const dummy_protocol_t::region_t &r2) {
    return !region_is_empty(region_intersection(r1, r2));
}

dummy_protocol_t::region_t drop_cpu_sharding(const dummy_protocol_t::region_t &r) {
    // TODO: This implementation is actually broken.
    return r;
}

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return a.keys == b.keys;
}

bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b) {
    return !(a == b);
}

dummy_protocol_t::region_t dummy_protocol_t::cpu_sharding_subspace(int subregion_number, DEBUG_VAR int num_cpu_shards) {
    rassert(subregion_number >= 0);
    rassert(subregion_number < num_cpu_shards);

    // We have a fairly worthless sharding scheme for the dummy protocol, for now.
    if (subregion_number == 0) {
        return region_t::universe();
    } else {
        return region_t::empty();
    }
}


dummy_protocol_t::store_t::store_t() : store_view_t<dummy_protocol_t>(dummy_protocol_t::region_t('a', 'z')), serializer(NULL) {
    initialize_empty();
}

dummy_protocol_t::store_t::store_t(serializer_t *_serializer,
                                   UNUSED cache_balancer_t *,
                                   UNUSED const std::string &,
                                   bool create,
                                   UNUSED perfmon_collection_t *, UNUSED context_t *,
                                   io_backender_t *, const base_path_t &) :
    store_view_t<dummy_protocol_t>(dummy_protocol_t::region_t('a', 'z')),
    serializer(_serializer) {
    if (create) {
        initialize_empty();
    } else {
        serializer_file_read_stream_t stream(serializer);
        archive_result_t res = deserialize(&stream, &metainfo);
        if (bad(res)) { throw fake_archive_exc_t(); }
        res = deserialize(&stream, &values);
        if (bad(res)) { throw fake_archive_exc_t(); }
        res = deserialize(&stream, &timestamps);
        if (bad(res)) { throw fake_archive_exc_t(); }
    }
}

dummy_protocol_t::store_t::~store_t() {
    if (serializer) {
        serializer_file_write_stream_t stream(serializer);
        write_message_t msg;
        msg << metainfo;
        msg << values;
        msg << timestamps;
        int res = send_write_message(&stream, &msg);
        if (res) { throw fake_archive_exc_t(); }
    }
}

void dummy_protocol_t::store_t::new_read_token(object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token_out) THROWS_NOTHING {
    assert_thread();
    fifo_enforcer_read_token_t token = main_token_source.enter_read();
    token_out->create(&main_token_sink, token);
}

void dummy_protocol_t::store_t::new_write_token(object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token_out) THROWS_NOTHING {
    assert_thread();
    fifo_enforcer_write_token_t token = main_token_source.enter_write();
    token_out->create(&main_token_sink, token);
}

void dummy_protocol_t::store_t::do_get_metainfo(order_token_t order_token,
                                                object_buffer_t<fifo_enforcer_sink_t::exit_read_t> *token,
                                                signal_t *interruptor,
                                                metainfo_t *out) THROWS_ONLY(interrupted_exc_t) {
    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(token);

    wait_interruptible(token->get(), interruptor);

    order_sink.check_out(order_token);

    if (rng.randint(2) == 0) {
        nap(rng.randint(10), interruptor);
    }
    metainfo_t res = metainfo.mask(get_region());
    *out = res;
}

void dummy_protocol_t::store_t::set_metainfo(const metainfo_t &new_metainfo,
                                             order_token_t order_token,
                                             object_buffer_t<fifo_enforcer_sink_t::exit_write_t> *token,
                                             signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(token);

    wait_interruptible(token->get(), interruptor);

    order_sink.check_out(order_token);

    if (rng.randint(2) == 0) {
        nap(rng.randint(10), interruptor);
    }

    metainfo.update(new_metainfo);
}

void dummy_protocol_t::store_t::read(DEBUG_ONLY(const metainfo_checker_t<dummy_protocol_t>& metainfo_checker, )
                                     const dummy_protocol_t::read_t &read,
                                     dummy_protocol_t::read_response_t *response,
                                     order_token_t order_token,
                                     read_token_pair_t *token_pair,
                                     signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));
    rassert(region_is_superset(get_region(), read.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token_pair->main_read_token);

        wait_interruptible(token_pair->main_read_token.get(), interruptor);
        order_sink.check_out(order_token);

        // We allow upper_metainfo domain to be smaller than the metainfo domain
#ifndef NDEBUG
        metainfo_checker.check_metainfo(metainfo.mask(metainfo_checker.get_domain()));
#endif

        if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        for (std::set<std::string>::iterator it = read.keys.keys.begin();
                it != read.keys.keys.end(); it++) {
            rassert(get_region().keys.count(*it) != 0);
            response->values[*it] = values[*it];
        }
    }
    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
}

void print_region(printf_buffer_t *buf, const dummy_protocol_t::region_t &region) {
    std::set<std::string>::const_iterator it = region.keys.begin(), e = region.keys.end();
    buf->appendf("{ ");
    for (; it != e; ++it) {
        buf->appendf("%s ", it->c_str());
    }
    buf->appendf("}");
}

void print_dummy_protocol_thing(printf_buffer_t *buf, const binary_blob_t &blob) {
    const uint8_t *data = static_cast<const uint8_t *>(blob.data());
    buf->appendf("'");
    for (size_t i = 0, e = blob.size(); i < e; ++i) {
        buf->appendf("%s%02x", i == 0 ? "" : " ", data[i]);
    }
    buf->appendf("'");
}

void print_metainfo(printf_buffer_t *buf, const region_map_t<dummy_protocol_t, binary_blob_t> &m) {
    region_map_t<dummy_protocol_t, binary_blob_t>::const_iterator it = m.begin(), e = m.end();
    buf->appendf("region_map_t(");
    for (; it != e; ++it) {
        print_region(buf, it->first);
        buf->appendf(" => ");
        print_dummy_protocol_thing(buf, it->second);
        buf->appendf(", ");
    }
    buf->appendf(")");
}

void dummy_protocol_t::store_t::write(DEBUG_ONLY(const metainfo_checker_t<dummy_protocol_t>& metainfo_checker, )
                                      const metainfo_t& new_metainfo,
                                      const dummy_protocol_t::write_t &write,
                                      dummy_protocol_t::write_response_t *response,
                                      UNUSED write_durability_t durability,
                                      transition_timestamp_t timestamp,
                                      order_token_t order_token,
                                      write_token_pair_t *token_pair,
                                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {

    rassert(region_is_superset(get_region(), metainfo_checker.get_domain()));
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));
    rassert(region_is_superset(get_region(), write.get_region()));

    {
        object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token_pair->main_write_token);

        wait_interruptible(token_pair->main_write_token.get(), interruptor);

        order_sink.check_out(order_token);

        // We allow upper_metainfo domain to be smaller than the metainfo domain
        rassert(metainfo_checker.get_domain() == metainfo.mask(metainfo_checker.get_domain()).get_domain());
#ifndef NDEBUG
        metainfo_checker.check_metainfo(metainfo.mask(metainfo_checker.get_domain()));
#endif

        if (rng.randint(2) == 0) nap(rng.randint(10));
        for (std::map<std::string, std::string>::const_iterator it = write.values.begin();
                it != write.values.end(); it++) {
            response->old_values[it->first] = values[it->first];
            values[it->first] = it->second;
            timestamps[it->first] = timestamp.timestamp_after();
        }

        metainfo.update(new_metainfo);
    }
    if (rng.randint(2) == 0) {
        nap(rng.randint(10));
    }
}

bool dummy_protocol_t::store_t::send_backfill(const region_map_t<dummy_protocol_t, state_timestamp_t> &start_point,
                                              send_backfill_callback_t<dummy_protocol_t> *send_backfill_cb,
                                              traversal_progress_combiner_t *progress,
                                              read_token_pair_t *token_pair,
                                              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    {
        scoped_ptr_t<traversal_progress_t> progress_owner(new dummy_protocol_t::backfill_progress_t(get_thread_id()));
        progress->add_constituent(&progress_owner);
    }

    rassert(region_is_superset(get_region(), start_point.get_domain()));

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t>::destruction_sentinel_t destroyer(&token_pair->main_read_token);

    wait_interruptible(token_pair->main_read_token.get(), interruptor);

    metainfo_t masked_metainfo = metainfo.mask(start_point.get_domain());
    if (send_backfill_cb->should_backfill(masked_metainfo)) {
        /* Make a copy so we can sleep and still have the correct semantics */
        std::map<std::string, std::string> values_snapshot = values;
        std::map<std::string, state_timestamp_t> timestamps_snapshot = timestamps;

        if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);

        token_pair->main_read_token.reset();

        if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
        for (region_map_t<dummy_protocol_t, state_timestamp_t>::const_iterator r_it  = start_point.begin();
                                                                               r_it != start_point.end();
                                                                               r_it++) {
            for (std::set<std::string>::iterator it = r_it->first.keys.begin();
                    it != r_it->first.keys.end(); it++) {
                if (timestamps_snapshot[*it] > r_it->second) {
                    dummy_protocol_t::backfill_chunk_t chunk;
                    chunk.key = *it;
                    chunk.value = values_snapshot[*it];
                    chunk.timestamp = timestamps_snapshot[*it];
                    send_backfill_cb->send_chunk(chunk, interruptor);
                }
                if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
            }
        }
        return true;
    } else {
        return false;
    }
}

void dummy_protocol_t::store_t::receive_backfill(const dummy_protocol_t::backfill_chunk_t &chunk, write_token_pair_t *token_pair, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token_pair->main_write_token);

    rassert(get_region().keys.count(chunk.key) != 0);

    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
    values[chunk.key] = chunk.value;
    timestamps[chunk.key] = chunk.timestamp;
    if (rng.randint(2) == 0) nap(rng.randint(10), interruptor);
}

void dummy_protocol_t::store_t::reset_data(const dummy_protocol_t::region_t &subregion,
                                           const metainfo_t &new_metainfo,
                                           write_token_pair_t *token_pair,
                                           UNUSED write_durability_t durability,
                                           signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    rassert(region_is_superset(get_region(), subregion));
    rassert(region_is_superset(get_region(), new_metainfo.get_domain()));

    object_buffer_t<fifo_enforcer_sink_t::exit_write_t>::destruction_sentinel_t destroyer(&token_pair->main_write_token);

    wait_interruptible(token_pair->main_write_token.get(), interruptor);

    rassert(region_is_superset(get_region(), subregion));
    for (std::set<std::string>::iterator it = subregion.keys.begin(); it != subregion.keys.end(); it++) {
        values[*it] = "";
        timestamps[*it] = state_timestamp_t::zero();
    }
    metainfo.update(new_metainfo);
}

void dummy_protocol_t::store_t::initialize_empty() {
    dummy_protocol_t::region_t region = get_region();
    for (std::set<std::string>::iterator it = region.keys.begin();
            it != region.keys.end(); it++) {
        values[*it] = "";
        timestamps[*it] = state_timestamp_t::zero();
    }
    metainfo = metainfo_t(region, binary_blob_t());
}

dummy_protocol_t::region_t a_thru_z_region() {
    dummy_protocol_t::region_t r;
    for (char c = 'a'; c <= 'z'; c++) {
        r.keys.insert(std::string(&c, 1));
    }
    return r;
}

std::string region_to_debug_str(dummy_protocol_t::region_t r) {
    std::string ret = "{ ";
    for (std::set<std::string>::iterator it  = r.keys.begin();
                                         it != r.keys.end();
                                         it++) {
        ret += *it;
        ret += " ";
    }
    ret += "}";

    return ret;
}

void debug_print(printf_buffer_t *buf, const dummy_protocol_t::region_t &region) {
    buf->appendf("dummy_region{");
    for (std::set<std::string>::const_iterator it = region.keys.begin(); it != region.keys.end(); ++it) {
        if (it != region.keys.begin()) {
            buf->appendf(", ");
        }
        const char *data = it->data();
        debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(data), it->size());
    }
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const dummy_protocol_t::write_t& write) {
    buf->appendf("dummy_write{");
    bool first = true;
    for (std::map<std::string, std::string>::const_iterator it = write.values.begin(); it != write.values.end(); ++it) {
        if (!first) {
            buf->appendf(", ");
        }
        first = false;
        const char *first_data = it->first.data();
        debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(first_data), it->first.size());
        buf->appendf(" => ");
        const char *second_data = it->second.data();
        debug_print_quoted_string(buf, reinterpret_cast<const uint8_t *>(second_data), it->second.size());
    }
    buf->appendf("}");
}

void debug_print(printf_buffer_t *buf, const dummy_protocol_t::backfill_chunk_t& chunk) {
    buf->appendf("dummy_chunk{key=");
    debug_print(buf, chunk.key);
    buf->appendf(", value=");
    debug_print(buf, chunk.value);
    buf->appendf(", timestamp=");
    debug_print(buf, chunk.timestamp);
    buf->appendf("}");
}



}   /* namespace mock */
