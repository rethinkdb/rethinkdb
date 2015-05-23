#include "unittest/dummy_namespace_interface.hpp"

#include "unittest/clustering_utils.hpp"
#include "rdb_protocol/env.hpp"

namespace unittest {

void dummy_performer_t::read(const read_t &read,
                             read_response_t *response,
                             DEBUG_VAR state_timestamp_t expected_timestamp,
                             signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    read_token_t token;
    store->new_read_token(&token);

#ifndef NDEBUG
    metainfo_checker_t metainfo_checker(store->get_region(),
        [&](const region_t &, const binary_blob_t &bb) {
            rassert(bb == binary_blob_t(expected_timestamp));
        });
#endif

    return store->read(DEBUG_ONLY(metainfo_checker, ) read, response, &token, interruptor);
}

void dummy_performer_t::read_outdated(const read_t &read,
                                      read_response_t *response,
                                      signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    read_token_t token;
    store->new_read_token(&token);

#ifndef NDEBUG
    metainfo_checker_t metainfo_checker(store->get_region(),
        [](const region_t &, const binary_blob_t &) { });
#endif

    return store->read(DEBUG_ONLY(metainfo_checker, ) read, response,
                       &token,
                       interruptor);
}

void dummy_performer_t::write(const write_t &write,
                              write_response_t *response,
                              state_timestamp_t timestamp,
                              order_token_t order_token) THROWS_NOTHING {
    cond_t non_interruptor;

#ifndef NDEBUG
    metainfo_checker_t metainfo_checker(store->get_region(),
        [](const region_t &, const binary_blob_t &) { });
#endif

    write_token_t token;
    store->new_write_token(&token);

    store->write(
            DEBUG_ONLY(metainfo_checker, )
            region_map_t<binary_blob_t>(store->get_region(), binary_blob_t(timestamp)),
            write, response, write_durability_t::SOFT, timestamp, order_token, &token, &non_interruptor);
}


dummy_timestamper_t::dummy_timestamper_t(dummy_performer_t *n,
                                         order_source_t *order_source)
    : next(n) {
    cond_t interruptor;

    read_token_t read_token;
    next->store->new_read_token(&read_token);

    region_map_t<binary_blob_t> metainfo = next->store->get_metainfo(
        order_source->check_in("dummy_timestamper_t").with_read_mode(),
        &read_token,
        next->store->get_region(),
        &interruptor);

    current_timestamp = state_timestamp_t::zero();
    metainfo.visit(metainfo.get_domain(),
        [&](const region_t &, const binary_blob_t &b) {
            state_timestamp_t region_ts = binary_blob_t::get<state_timestamp_t>(b);
            current_timestamp = std::max(current_timestamp, region_ts);
        });
}

void dummy_timestamper_t::read(const read_t &read, read_response_t *response, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
    order_sink.check_out(otok);
    next->read(read, response, current_timestamp, interruptor);
}

void dummy_timestamper_t::write(const write_t &write, write_response_t *response, order_token_t otok) THROWS_NOTHING {
    order_sink.check_out(otok);
    current_timestamp = current_timestamp.next();
    next->write(write, response, current_timestamp, otok);
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

            read_token_t read_token;
            stores[i]->new_read_token(&read_token);

            region_map_t<binary_blob_t> metainfo = stores[i]->get_metainfo(
                order_source->check_in("dummy_namespace_interface_t::"
                    "dummy_namespace_interface_t (get_metainfo)").with_read_mode(),
                &read_token,
                shards[i],
                &interruptor);

            rassert(metainfo.get_domain() == shards[i]);
            metainfo.visit(shards[i], [&](const region_t &, const binary_blob_t &b) {
                guarantee(b.size() == 0);
            });

            write_token_t write_token;
            stores[i]->new_write_token(&write_token);

            stores[i]->set_metainfo(
                    region_map_t<binary_blob_t>(
                        shards[i], binary_blob_t(state_timestamp_t::zero())),
                    order_source->check_in("dummy_namespace_interface_t::"
                        "dummy_namespace_interface_t (set_metainfo)"),
                    &write_token,
                    write_durability_t::SOFT,
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
