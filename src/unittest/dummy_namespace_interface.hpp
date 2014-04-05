// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_
#define UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_

#include <vector>

#include "utils.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

#include "concurrency/pmap.hpp"
#include "containers/scoped.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

namespace unittest {

class dummy_performer_t {

public:
    explicit dummy_performer_t(store_view_t *s) :
        store(s) { }

    void read(const read_t &read,
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

    void read_outdated(const read_t &read,
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

    void write(const write_t &write,
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

    order_source_t bs_outdated_read_source;

    store_view_t *store;
};

struct dummy_timestamper_t {

public:
    dummy_timestamper_t(dummy_performer_t *n, order_source_t *order_source)
        : next(n), current_timestamp(state_timestamp_t::zero()) {
        cond_t interruptor;

        object_buffer_t<fifo_enforcer_sink_t::exit_read_t> read_token;
        next->store->new_read_token(&read_token);

        region_map_t<binary_blob_t> metainfo;
        next->store->do_get_metainfo(order_source->check_in("dummy_timestamper_t").with_read_mode(),
                                     &read_token, &interruptor, &metainfo);

        for (region_map_t<binary_blob_t>::iterator it  = metainfo.begin();
             it != metainfo.end();
             ++it) {
            rassert(binary_blob_t::get<state_timestamp_t>(it->second) == current_timestamp);
        }
    }

    void read(const read_t &read, read_response_t *response, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        order_sink.check_out(otok);
        next->read(read, response, current_timestamp, otok, interruptor);
    }

    void write(const write_t &write, write_response_t *response, order_token_t otok) THROWS_NOTHING {
        order_sink.check_out(otok);
        transition_timestamp_t transition_timestamp = transition_timestamp_t::starting_from(current_timestamp);
        current_timestamp = transition_timestamp.timestamp_after();
        next->write(write, response, transition_timestamp, otok);
    }

private:
    dummy_performer_t *next;
    state_timestamp_t current_timestamp;
    order_sink_t order_sink;
};

class dummy_sharder_t {

public:
    struct shard_t {
        shard_t(dummy_timestamper_t *ts, dummy_performer_t *pf, region_t r) :
            timestamper(ts), performer(pf), region(r) { }
        dummy_timestamper_t *timestamper;
        dummy_performer_t *performer;
        region_t region;
    };

    explicit dummy_sharder_t(std::vector<shard_t> _shards,
                             rdb_context_t *_ctx)
        : shards(_shards), ctx(_ctx) { }

    void read(const read_t &read, read_response_t *response, order_token_t tok, signal_t *interruptor) {
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

    void read_outdated(const read_t &read, read_response_t *response, signal_t *interruptor) {
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

    void write(const write_t &write, write_response_t *response, order_token_t tok, signal_t *interruptor) {
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

private:
    std::vector<shard_t> shards;
    rdb_context_t *ctx;
};

class dummy_namespace_interface_t : public namespace_interface_t {
public:
    dummy_namespace_interface_t(std::vector<region_t>
            shards, store_view_t **stores, order_source_t
            *order_source, rdb_context_t *_ctx)
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
            {
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

            dummy_performer_t *performer = new dummy_performer_t(stores[i]);
            performers.push_back(performer);
            dummy_timestamper_t *timestamper = new dummy_timestamper_t(performer, order_source);
            timestampers.push_back(timestamper);
            shards_of_this_db.push_back(dummy_sharder_t::shard_t(timestamper, performer, shards[i]));
        }

        sharder.init(new dummy_sharder_t(shards_of_this_db, ctx));
    }

    void read(const read_t &read, read_response_t *response, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
        return sharder->read(read, response, tok, interruptor);
    }

    void read_outdated(const read_t &read, read_response_t *response, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
        return sharder->read_outdated(read, response, interruptor);
    }

    void write(const write_t &write, write_response_t *response, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
        return sharder->write(write, response, tok, interruptor);
    }

private:
    boost::ptr_vector<dummy_performer_t> performers;
    boost::ptr_vector<dummy_timestamper_t> timestampers;
    scoped_ptr_t<dummy_sharder_t> sharder;
    rdb_context_t *ctx;
};

}   /* namespace unittest */

#endif /* UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_ */
