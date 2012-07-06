#ifndef UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_
#define UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_

#include "utils.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>

#include "concurrency/pmap.hpp"
#include "mock/clustering_utils.hpp"
#include "mock/unittest_utils.hpp"
#include "protocol_api.hpp"
#include "timestamps.hpp"

namespace unittest {

template<class protocol_t>
class dummy_performer_t {

public:
    explicit dummy_performer_t(boost::shared_ptr<store_view_t<protocol_t> > s) :
        store(s) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read,
                                              DEBUG_ONLY_VAR state_timestamp_t expected_timestamp,
                                              signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
        store->new_read_token(read_token);

#ifndef NDEBUG
        mock::equality_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback((binary_blob_t(expected_timestamp)));
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, store->get_region());
#endif

        return store->read(DEBUG_ONLY(metainfo_checker, ) read, order_token_t::ignore, read_token, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, transition_timestamp_t transition_timestamp) THROWS_NOTHING {
        cond_t non_interruptor;

#ifndef NDEBUG
        mock::equality_metainfo_checker_callback_t<protocol_t> metainfo_checker_callback(binary_blob_t(transition_timestamp.timestamp_before()));
        metainfo_checker_t<protocol_t> metainfo_checker(&metainfo_checker_callback, store->get_region());
#endif

        boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
        store->new_write_token(write_token);

        return store->write(
            DEBUG_ONLY(metainfo_checker, )
            region_map_t<protocol_t, binary_blob_t>(store->get_region(), binary_blob_t(transition_timestamp.timestamp_after())),
            write, transition_timestamp,
            order_token_t::ignore,
            write_token, &non_interruptor
            );
    }

    boost::shared_ptr<store_view_t<protocol_t> > store;
};

template<class protocol_t>
struct dummy_timestamper_t {

public:
    explicit dummy_timestamper_t(dummy_performer_t<protocol_t> *n) :
        next(n), current_timestamp(state_timestamp_t::zero())
    {
        cond_t interruptor;

        boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
        next->store->new_read_token(read_token);

        region_map_t<protocol_t, binary_blob_t> metainfo = next->store->get_metainfo(order_token_t::ignore, read_token, &interruptor);

        for (typename region_map_t<protocol_t, binary_blob_t>::iterator it  = metainfo.begin();
                                                                        it != metainfo.end();
                                                                        it++) {
            rassert(binary_blob_t::get<state_timestamp_t>(it->second) == current_timestamp);
        }
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t otok, signal_t *interruptor) THROWS_ONLY(interrupted_exc_t) {
        order_sink.check_out(otok);
        return next->read(read, current_timestamp, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t otok) THROWS_NOTHING {
        order_sink.check_out(otok);
        transition_timestamp_t transition_timestamp = transition_timestamp_t::starting_from(current_timestamp);
        current_timestamp = transition_timestamp.timestamp_after();
        return next->write(write, transition_timestamp);
    }

private:
    dummy_performer_t<protocol_t> *next;
    state_timestamp_t current_timestamp;
    order_sink_t order_sink;
};

template<class protocol_t>
class dummy_sharder_t {

public:
    struct timestamper_and_region_t {
        timestamper_and_region_t(dummy_timestamper_t<protocol_t> *ts, typename protocol_t::region_t r) :
            timestamper(ts), region(r) { }
        dummy_timestamper_t<protocol_t> *timestamper;
        typename protocol_t::region_t region;
    };

    explicit dummy_sharder_t(std::vector<timestamper_and_region_t> _timestampers)
        : timestampers(_timestampers) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
        std::vector<typename protocol_t::read_response_t> responses;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            typename protocol_t::region_t ixn = region_intersection(timestampers[i].region, read.get_region());
            if (!region_is_empty(ixn)) {
                typename protocol_t::read_t subread = read.shard(ixn);
                typename protocol_t::read_response_t subresponse = timestampers[i].timestamper->read(subread, tok, interruptor);
                responses.push_back(subresponse);
                if (interruptor->is_pulsed()) throw interrupted_exc_t();
            }
        }
        typename protocol_t::temporary_cache_t cache;
        return read.unshard(responses, &cache);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok, signal_t *interruptor) {
        if (interruptor->is_pulsed()) throw interrupted_exc_t();
        std::vector<typename protocol_t::write_response_t> responses;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            typename protocol_t::region_t ixn = region_intersection(timestampers[i].region, write.get_region());

            if (!region_is_empty(ixn)) {
                typename protocol_t::write_t subwrite = write.shard(ixn);
                typename protocol_t::write_response_t subresponse = timestampers[i].timestamper->write(subwrite, tok);
                responses.push_back(subresponse);
                if (interruptor->is_pulsed()) throw interrupted_exc_t();
            }
        }
        typename protocol_t::temporary_cache_t cache;
        return write.unshard(responses, &cache);
    }

private:
    std::vector<timestamper_and_region_t> timestampers;
};

template<class protocol_t>
class dummy_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    dummy_namespace_interface_t(std::vector<typename protocol_t::region_t> shards, std::vector<boost::shared_ptr<store_view_t<protocol_t> > > stores) {
        /* Make sure shards are non-overlapping and stuff */
        {
            typename protocol_t::region_t join;
            region_join_result_t result = region_join(shards, &join);
            if (result != REGION_JOIN_OK) {
                throw std::runtime_error("bad region join");
            }
        }
        rassert(stores.size() == shards.size());

        std::vector<typename dummy_sharder_t<protocol_t>::timestamper_and_region_t> shards_of_this_db;
        for (int i = 0; i < (int)shards.size(); i++) {
            /* Initialize metadata everywhere */
            {
                cond_t interruptor;

                boost::scoped_ptr<fifo_enforcer_sink_t::exit_read_t> read_token;
                stores[i]->new_read_token(read_token);

                region_map_t<protocol_t, binary_blob_t> metadata = stores[i]->get_metainfo(order_token_t::ignore, read_token, &interruptor);

                rassert(metadata.get_domain() == shards[i]);
                for (typename region_map_t<protocol_t, binary_blob_t>::const_iterator it  = metadata.begin();
                                                                                      it != metadata.end();
                                                                                      it++) {
                    rassert(it->second.size() == 0);
                }

                boost::scoped_ptr<fifo_enforcer_sink_t::exit_write_t> write_token;
                stores[i]->new_write_token(write_token);

                stores[i]->set_metainfo(
                    region_map_transform<protocol_t, state_timestamp_t, binary_blob_t>(
                        region_map_t<protocol_t, state_timestamp_t>(shards[i], state_timestamp_t::zero()),
                        &binary_blob_t::make<state_timestamp_t>
                        ),
                    order_token_t::ignore,
                    write_token,
                    &interruptor);
            }

            dummy_performer_t<protocol_t> *performer = new dummy_performer_t<protocol_t>(stores[i]);
            performers.push_back(performer);
            dummy_timestamper_t<protocol_t> *timestamper = new dummy_timestamper_t<protocol_t>(performer);
            timestampers.push_back(timestamper);
            shards_of_this_db.push_back(typename dummy_sharder_t<protocol_t>::timestamper_and_region_t(timestamper, shards[i]));
        }

        sharder.reset(new dummy_sharder_t<protocol_t>(shards_of_this_db));
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
        return sharder->read(read, tok, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok, signal_t *interruptor) THROWS_ONLY(cannot_perform_query_exc_t, interrupted_exc_t) {
        return sharder->write(write, tok, interruptor);
    }

private:
    boost::ptr_vector<dummy_performer_t<protocol_t> > performers;
    boost::ptr_vector<dummy_timestamper_t<protocol_t> > timestampers;
    boost::scoped_ptr<dummy_sharder_t<protocol_t> > sharder;
};

}   /* namespace unittest */

#endif /* UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP_ */
