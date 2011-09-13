#ifndef __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__
#define __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__

#include "utils.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "concurrency/pmap.hpp"
#include "namespace_interface.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "timestamps.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

template<class protocol_t>
struct dummy_timestamper_t {

public:
    explicit dummy_timestamper_t(typename protocol_t::store_t *_next)
        : next(_next), timestamp(state_timestamp_t::zero()) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        cond_t interruptor;
        return next->read(read, tok, &interruptor);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        cond_t interruptor;
        transition_timestamp_t ts = transition_timestamp_t::starting_from(timestamp);
        timestamp = ts.timestamp_after();
        return next->write(write, ts, tok, &interruptor);
    }

private:
    typename protocol_t::store_t *next;
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

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(read.get_region())) {
                regions.push_back(timestampers[i].region.intersection(read.get_region()));
                destinations.push_back(i);
            }
        }
        rassert(read.get_region().covered_by(regions));
        std::vector<typename protocol_t::read_t> subreads = read.shard(regions);
        rassert(subreads.size() == regions.size());
        std::vector<typename protocol_t::read_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subreads[i].get_region()));
            responses.push_back(timestampers[destinations[i]].timestamper->read(subreads[i], tok));
        }
        typename protocol_t::temporary_cache_t cache;
        return read.unshard(responses, &cache);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(write.get_region())) {
                regions.push_back(timestampers[i].region.intersection(write.get_region()));
                destinations.push_back(i);
            }
        }
        rassert(write.get_region().covered_by(regions));
        std::vector<typename protocol_t::write_t> subwrites = write.shard(regions);
        rassert(subwrites.size() == regions.size());
        std::vector<typename protocol_t::write_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subwrites[i].get_region()));
            responses.push_back(timestampers[destinations[i]].timestamper->write(subwrites[i], tok));
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
    dummy_namespace_interface_t(std::vector<typename protocol_t::region_t> shards, std::vector<typename protocol_t::store_t *> stores) {

        rassert(stores.size() == shards.size());

        std::vector<typename dummy_sharder_t<protocol_t>::timestamper_and_region_t> shards_of_this_db;

        for (int i = 0; i < (int)shards.size(); i++) {

            for (int j = 0; j < (int)shards.size(); j++) {
                if (i > j) {
                    rassert(!shards[i].overlaps(shards[j]));
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

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        return sharder->read(read, tok);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        return sharder->write(write, tok);
    }

private:
    boost::ptr_vector<dummy_timestamper_t<protocol_t> > timestampers;
    boost::scoped_ptr<dummy_sharder_t<protocol_t> > sharder;
};

/* Currently, the protocol interface doesn't specify how `store_t`s get created.
`run_with_dummy_namespace_interface` is to make it easier to test new protocols;
it assumes that `protocol_t::store_t` supports the following API:

    static void create(serializer_t *, region_t);
    store_t(serializer_t *, region_t);
    ~store_t();

If your `protocol_t` doesn't match that interface, you'll have to construct a
`dummy_namespace_interface_t` manually instead of using
`run_with_dummy_namespace_interface`. */

template<class protocol_t>
void run_with_dummy_namespace_interface(
        std::vector<typename protocol_t::region_t> shards,
        boost::function<void(namespace_interface_t<protocol_t> *)> fun) {

    temp_file_t db_file("/tmp/rdb_unittest.XXXXXX");

    /* Set up serializer */

    standard_serializer_t::create(
        standard_serializer_t::dynamic_config_t(),
        standard_serializer_t::private_dynamic_config_t(db_file.name()),
        standard_serializer_t::static_config_t()
        );

    standard_serializer_t serializer(
        /* Extra parentheses are necessary so C++ doesn't interpret this as
        a declaration of a function called `serializer`. WTF, C++? */
        (standard_serializer_t::dynamic_config_t()),
        standard_serializer_t::private_dynamic_config_t(db_file.name())
        );

    /* Set up multiplexer */

    std::vector<standard_serializer_t *> multiplexer_files;
    multiplexer_files.push_back(&serializer);

    serializer_multiplexer_t::create(multiplexer_files, shards.size());

    serializer_multiplexer_t multiplexer(multiplexer_files);
    rassert(multiplexer.proxies.size() == shards.size());

    /* Set up stores */

    boost::ptr_vector<typename protocol_t::store_t> store_storage;   // to call destructors
    std::vector<typename protocol_t::store_t *> stores;

    for (int i = 0; i < (int)shards.size(); i++) {

        serializer_t *serializer = multiplexer.proxies[i];

        protocol_t::store_t::create(serializer, shards[i]);

        typename protocol_t::store_t *store =
            new typename protocol_t::store_t(serializer, shards[i]);
        store_storage.push_back(store);
        stores.push_back(store);
    }

    /* Set up namespace interface */

    dummy_namespace_interface_t<protocol_t> nsi(shards, stores);

    fun(&nsi);
}

}   /* namespace unittest */

#endif /* __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__ */
