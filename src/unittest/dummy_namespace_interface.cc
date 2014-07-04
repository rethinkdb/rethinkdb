#include "unittest/dummy_namespace_interface.hpp"

#include "unittest/clustering_utils.hpp"

namespace unittest {

void dummy_performer_t::read(const read_t &read,
                             read_response_t *response,
                             DEBUG_VAR state_timestamp_t expected_timestamp,
                             order_token_t order_token,
                             signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    read_token_pair_t token_pair;
    store->new_read_token_pair(&token_pair);

#ifndef NDEBUG
    equality_metainfo_checker_callback_t metainfo_checker_callback((binary_blob_t(expected_timestamp)));
    metainfo_checker_t metainfo_checker(&metainfo_checker_callback, store->get_region());
#endif

    return store->read(DEBUG_ONLY(metainfo_checker, ) read, response, order_token, &token_pair, interruptor);
}

void dummy_performer_t::read_outdated(const read_t &read,
                                      read_response_t *response,
                                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    read_token_pair_t token_pair;
    store->new_read_token_pair(&token_pair);

#ifndef NDEBUG
    trivial_metainfo_checker_callback_t metainfo_checker_callback;
    metainfo_checker_t metainfo_checker(&metainfo_checker_callback, store->get_region());
#endif

    return store->read(DEBUG_ONLY(metainfo_checker, ) read, response,
                       bs_outdated_read_source.check_in("dummy_performer_t::read_outdated").with_read_mode(),
                       &token_pair,
                       interruptor);
}

void dummy_performer_t::write(const write_t &write,
                              write_response_t *response,
                              transition_timestamp_t transition_timestamp,
                              order_token_t order_token) THROWS_NOTHING {
    cond_t non_interruptor;

#ifndef NDEBUG
    equality_metainfo_checker_callback_t metainfo_checker_callback(binary_blob_t(transition_timestamp.timestamp_before()));
    metainfo_checker_t metainfo_checker(&metainfo_checker_callback, store->get_region());
#endif

    write_token_pair_t token_pair;
    store->new_write_token_pair(&token_pair);

    store->write(
            DEBUG_ONLY(metainfo_checker, )
            region_map_t<binary_blob_t>(store->get_region(), binary_blob_t(transition_timestamp.timestamp_after())),
            write, response, write_durability_t::SOFT, transition_timestamp, order_token, &token_pair, &non_interruptor);
}


dummy_timestamper_t::dummy_timestamper_t(dummy_performer_t *n,
                                         order_source_t *order_source)
    : next(n) {
    cond_t interruptor;

    object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
    next->store->new_read_token(&read_token);

    region_map_t<binary_blob_t> metainfo;
    next->store->do_get_metainfo(order_source->check_in("dummy_timestamper_t").with_read_mode(),
                                 &read_token, &interruptor, &metainfo);

    current_timestamp = state_timestamp_t::zero();
    for (typename region_map_t<binary_blob_t>::iterator it  = metainfo.begin();
                                                        it != metainfo.end();
                                                        it++) {
        state_timestamp_t region_timestamp
            = binary_blob_t::get<state_timestamp_t>(it->second);
        if (region_timestamp > current_timestamp) {
            current_timestamp = region_timestamp;
        }
    }
}

void dummy_timestamper_t::read(const read_t &read, read_response_t *response, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    order_sink.check_out(otok);
    next->read(read, response, current_timestamp, otok, interruptor);
}

void dummy_timestamper_t::write(const write_t &write, write_response_t *response, order_token_t otok) THROWS_NOTHING {
    order_sink.check_out(otok);
    transition_timestamp_t transition_timestamp = transition_timestamp_t::starting_from(current_timestamp);
    current_timestamp = transition_timestamp.timestamp_after();
    next->write(write, response, transition_timestamp, otok);
}

void dummy_sharder_t::read(const read_t &read, read_response_t *response, order_token_t tok, signal_t *interruptor) {
    if (interruptor->is_pulsed()) { throw interrupted_exc_t(); }

    std::vector<read_response_t> responses;
    responses.reserve(shards.size());

    for (auto it = shards.begin(); it != shards.end(); ++it) {
        read_t subread;
        if (read.shard(it->region, &subread)) {
            responses.push_back(read_response_t());
            it->timestamper->read(subread, &responses.back(), tok, interruptor);
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }
    }

    read.unshard(responses.data(), responses.size(), response, ctx, interruptor);
}

void dummy_sharder_t::read_outdated(const read_t &read, read_response_t *response, signal_t *interruptor) {
    if (interruptor->is_pulsed()) { throw interrupted_exc_t(); }

    std::vector<read_response_t> responses;
    responses.reserve(shards.size());

    for (auto it = shards.begin(); it != shards.end(); ++it) {
        read_t subread;
        if (read.shard(it->region, &subread)) {
            responses.push_back(read_response_t());
            it->performer->read_outdated(subread, &responses.back(), interruptor);
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }
    }

    read.unshard(responses.data(), responses.size(), response, ctx, interruptor);
}

void dummy_sharder_t::write(const write_t &write, write_response_t *response, order_token_t tok, signal_t *interruptor) {
    if (interruptor->is_pulsed()) { throw interrupted_exc_t(); }

    std::vector<write_response_t> responses;
    responses.reserve(shards.size());

    for (auto it = shards.begin(); it != shards.end(); ++it) {
        write_t subwrite;
        if (write.shard(it->region, &subwrite)) {
            responses.push_back(write_response_t());
            it->timestamper->write(subwrite, &responses.back(), tok);
            if (interruptor->is_pulsed()) {
                throw interrupted_exc_t();
            }
        }
    }

    write.unshard(responses.data(), responses.size(), response, ctx, interruptor);
}

dummy_namespace_interface_t::
dummy_namespace_interface_t(std::vector<region_t> shards,
                            store_view_t *const *stores, order_source_t
                            *order_source, rdb_context_t *_ctx,
                            bool initialize_metadata)
    : ctx(_ctx)
{
    /* Make sure shards are non-overlapping and stuff */
    {
        region_t join;
        region_join_result_t result = region_join(shards, &join);
        if (result != REGION_JOIN_OK) {
            throw std::runtime_error("bad region join");
        }
    }

    std::vector<dummy_sharder_t::shard_t> shards_of_this_db;
    for (size_t i = 0; i < shards.size(); ++i) {
        /* Initialize metadata everywhere */
        if (initialize_metadata) {
            cond_t interruptor;

            object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
            stores[i]->new_read_token(&read_token);

            region_map_t<binary_blob_t> metadata;
            stores[i]->do_get_metainfo(order_source->check_in("dummy_namespace_interface_t::dummy_namespace_interface_t (do_get_metainfo)").with_read_mode(),
                                       &read_token, &interruptor, &metadata);

            rassert(metadata.get_domain() == shards[i]);
            for (region_map_t<binary_blob_t>::const_iterator it = metadata.begin();
                 it != metadata.end();
                 ++it) {
                rassert(it->second.size() == 0);
            }

            object_buffer_t<fifo_enforcer_sink_t::exit_write_t> write_token;
            stores[i]->new_write_token(&write_token);

            stores[i]->set_metainfo(
                    region_map_transform<state_timestamp_t, binary_blob_t>(
                            region_map_t<state_timestamp_t>(shards[i], state_timestamp_t::zero()),
                            &binary_blob_t::make<state_timestamp_t>
                                                                           ),
                    order_source->check_in("dummy_namespace_interface_t::dummy_namespace_interface_t (set_metainfo)"),
                    &write_token,
                    &interruptor);
        }

        performers.push_back(make_scoped<dummy_performer_t>(stores[i]));
        timestampers.push_back(make_scoped<dummy_timestamper_t>(performers.back().get(), order_source));
        shards_of_this_db.push_back(dummy_sharder_t::shard_t(timestampers.back().get(), performers.back().get(), shards[i]));
    }

    sharder.init(new dummy_sharder_t(shards_of_this_db, ctx));
}

std::set<region_t> dummy_namespace_interface_t::get_sharding_scheme()
    THROWS_ONLY(cannot_perform_query_exc_t) {
    std::set<region_t> s;
    s.insert(region_t::universe());
    return s;
}




}  // namespace unittest
