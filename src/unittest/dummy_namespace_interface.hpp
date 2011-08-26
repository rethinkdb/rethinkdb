#ifndef __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__
#define __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__

#include "utils.hpp"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include "namespace_interface.hpp"
#include "serializer/config.hpp"
#include "serializer/translator.hpp"
#include "concurrency/pmap.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

template<class protocol_t>
struct dummy_parallelizer_t {

public:
    dummy_parallelizer_t(std::vector<typename protocol_t::store_t *> _children) :
        children(_children), random_step(0) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        std::vector<typename protocol_t::read_t> reads = read->parallelize(children.size());
        std::vector<typename protocol_t::read_response_t> responses(reads.size());
        pmap(reads.size(), boost::bind(&dummy_parallelizer_t::read_one, this, random_step, tok, &reads, _1, &responses));
        return protocol_t::read_response_t::unparallelize(responses);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, repli_timestamp_t timestamp, order_token_t tok) {
        typename protocol_t::write_response_t response;
        pmap(children.size(), boost::bind(&dummy_parallelizer_t::write_one, this, timestamp, write, tok, _1, &response));
        return response;
    }

private:
    void read_one(int step, order_token_t tok, std::vector<typename protocol_t::read_t> *reads, int i, std::vector<typename protocol_t::read_response_t> *responses_out) {
        (*responses_out)[i] = children[(step + i) % children.size()]->read((*reads)[i], tok);
    }

    void write_one(repli_timestamp_t timestamp, typename protocol_t::write_t write, order_token_t tok, int i, typename protocol_t::write_response_t *response_out) {
        typename protocol_t::write_response_t resp = children[i]->write(write, timestamp, tok);
        if (i == 0) *response_out = resp;
    }

    std::vector<typename protocol_t::store_t *> children;
    int random_step;
};

template<class protocol_t>
struct dummy_timestamper_t {

public:
    dummy_timestamper_t(dummy_parallelizer_t<protocol_t> *_next)
	: next(_next), timestamp(repli_timestamp_t::distant_past) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        return next->read(read, tok);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        repli_timestamp_t ts = timestamp;
        timestamp = timestamp.next();
        return next->write(write, ts, tok);
    }

private:
    dummy_parallelizer_t<protocol_t> *next;
    repli_timestamp_t timestamp;
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

    dummy_sharder_t(std::vector<timestamper_and_region_t> _timestampers)
	: timestampers(_timestampers) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(read->get_region())) {
                regions.push_back(timestampers[i].region.intersection(read->get_region()));
                destinations.push_back(i);
            }
        }
        std::vector<typename protocol_t::read_t> subreads = read->shard(regions);
        rassert(subreads.size() == regions.size());
        std::vector<typename protocol_t::read_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subreads[i]->get_region()));
            responses.push_back(timestampers[destinations[i]].timestamper->read(subreads[i], tok));
        }
        return protocol_t::read_response_t::unshard(responses);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        typename protocol_t::region_t write_region = write->get_region();
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(write_region)) {
                regions.push_back(timestampers[i].region);
                destinations.push_back(i);
            }
        }
        std::vector<typename protocol_t::write_t> subwrites = write->shard(regions);
        rassert(subwrites.size() == regions.size());
        std::vector<typename protocol_t::write_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subwrites[i]->get_region()));
            responses.push_back(timestampers[destinations[i]].timestamper->write(subwrites[i], tok));
        }
        return protocol_t::write_response_t::unshard(responses);
    }

private:
    std::vector<timestamper_and_region_t> timestampers;
};

template<class protocol_t>
class dummy_namespace_interface_t :
    public namespace_interface_t<protocol_t>
{
public:
    dummy_namespace_interface_t(std::vector<typename protocol_t::region_t> shards, int repli_factor, std::vector<typename protocol_t::store_t *> stores) {

        rassert(stores.size() == repli_factor * shards.size());

        std::vector<typename dummy_sharder_t<protocol_t>::timestamper_and_region_t> shards_of_this_db;

        for (int i = 0; i < (int)shards.size(); i++) {

            std::vector<typename protocol_t::store_t *> mirrors_of_this_shard;

            for (int j = 0; j < repli_factor; j++) {

                typename protocol_t::store_t *store = stores[i*repli_factor + j];
                rassert(store->get_region() == shards[i]);
                mirrors_of_this_shard.push_back(store);
            }

            dummy_parallelizer_t<protocol_t> *parallelizer =
                new dummy_parallelizer_t<protocol_t>(mirrors_of_this_shard);
            parallelizers.push_back(parallelizer);

            dummy_timestamper_t<protocol_t> *timestamper =
                new dummy_timestamper_t<protocol_t>(parallelizer);
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
    boost::ptr_vector<dummy_parallelizer_t<protocol_t> > parallelizers;
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

class bob_bob_t {
    char *filename;
public:
    bob_bob_t(const char *tmpl) {
        size_t len = strlen(tmpl);
        filename = new char[len+1];
        memcpy(filename, tmpl, len+1);
        int fd = mkstemp(filename);
        guarantee_err(fd != -1, "Couldn't create a temporary file");
        close(fd);
    }

    const char *name() { return filename; }

    ~bob_bob_t() {
        unlink(filename);
        delete [] filename;
    }
};

template<class protocol_t>
void run_with_dummy_namespace_interface(
        std::vector<typename protocol_t::region_t> shards,
        int repli_factor,
        boost::function<void(namespace_interface_t<protocol_t> *)> fun) {

    bob_bob_t db_file("/tmp/rdb_unittest.XXXXXX");

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

    serializer_multiplexer_t::create(multiplexer_files, shards.size() * repli_factor);

    serializer_multiplexer_t multiplexer(multiplexer_files);
    rassert(multiplexer.proxies.size() == shards.size() * repli_factor);

    /* Set up stores */

    boost::ptr_vector<typename protocol_t::store_t> store_storage;   // to call destructors
    std::vector<typename protocol_t::store_t *> stores;

    for (int i = 0; i < (int)shards.size(); i++) {
        for (int j = 0; j < repli_factor; j++) {

            serializer_t *serializer = multiplexer.proxies[i*repli_factor+j];

            protocol_t::store_t::create(serializer, shards[i]);

            typename protocol_t::store_t *store =
                new typename protocol_t::store_t(serializer, shards[i]);
            store_storage.push_back(store);
            stores.push_back(store);
        }
    }

    /* Set up namespace interface */

    dummy_namespace_interface_t<protocol_t> nsi(shards, repli_factor, stores);

    fun(&nsi);
}

}   /* namespace unittest */

#endif /* __UNITTEST_DUMMY_NAMESPACE_INTERFACE_HPP__ */
