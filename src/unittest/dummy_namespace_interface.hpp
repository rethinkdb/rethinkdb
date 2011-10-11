#ifndef __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__
#define __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__

#include "utils.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "concurrency/pmap.hpp"
#include "protocol_api.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "timestamps.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

template<class protocol_t>
struct dummy_timestamper_t {

public:
    explicit dummy_timestamper_t(boost::shared_ptr<store_view_t<protocol_t> > next_)
        : next(next_), timestamp(state_timestamp_t::zero()) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {
        return next->read(read, timestamp, tok, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        transition_timestamp_t ts = transition_timestamp_t::starting_from(timestamp);
        timestamp = ts.timestamp_after();
        return next->write(write, ts, tok);
    }

private:
    boost::shared_ptr<store_view_t<protocol_t> > next;
    state_timestamp_t timestamp;
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

        rassert(stores.size() == shards.size());

        std::vector<typename dummy_sharder_t<protocol_t>::timestamper_and_region_t> shards_of_this_db;

        for (int i = 0; i < (int)shards.size(); i++) {

            for (int j = 0; j < (int)shards.size(); j++) {
                if (i > j) {
                    rassert(!region_overlaps(shards[i], shards[j]));
                }
            }

            dummy_timestamper_t<protocol_t> *timestamper =
                new dummy_timestamper_t<protocol_t>(stores[i]);
            timestampers.push_back(timestamper);

            shards_of_this_db.push_back(
                typename dummy_sharder_t<protocol_t>::timestamper_and_region_t(timestamper, shards[i])
                );
        }

        sharder.reset(new dummy_sharder_t<protocol_t>(shards_of_this_db));
    }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok, signal_t *interruptor) {
        return sharder->read(read, tok, interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok, signal_t *interruptor) {
        return sharder->write(write, tok, interruptor);
    }

private:
    boost::ptr_vector<dummy_timestamper_t<protocol_t> > timestampers;
    boost::scoped_ptr<dummy_sharder_t<protocol_t> > sharder;
};

}   /* namespace unittest */

#endif /* __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__ */
