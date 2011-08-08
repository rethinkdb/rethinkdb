#ifndef __SERVER_DUMMY_NAMESPACE_INTERFACE_HPP__
#define __SERVER_DUMMY_NAMESPACE_INTERFACE_HPP__

#include "server/namespace_interface.hpp"
#include "serializer/log/log_serializer.hpp"
#include "serializer/translator.hpp"
#include "utils.hpp"
#include <boost/ptr_container/ptr_vector.hpp>
#include "concurrency/pmap.hpp"

template<class protocol_t>
struct dummy_parallelizer_t {

public:
    dummy_parallelizer_t(std::vector<typename protocol_t::store_t *> children) :
        children(children), random_step(0) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        std::vector<typename protocol_t::read_t> reads = read.parallelize(children.size());
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
    dummy_timestamper_t(dummy_parallelizer_t<protocol_t> *next) : next(next), timestamp(repli_timestamp_t::distant_past) { }

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

    dummy_sharder_t(std::vector<timestamper_and_region_t> timestampers) :
        timestampers(timestampers) { }

    typename protocol_t::read_response_t read(typename protocol_t::read_t read, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(read.get_region())) {
                regions.push_back(timestampers[i].region.intersection(read.get_region()));
                destinations.push_back(i);
            }
        }
        std::vector<typename protocol_t::read_t> subreads = read.shard(regions);
        rassert(subreads.size() == regions.size());
        std::vector<typename protocol_t::read_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subreads[i].get_region()));
            responses.push_back(timestampers[destinations[i]].timestamper->read(subreads[i], tok));
        }
        return protocol_t::read_response_t::unshard(responses);
    }

    typename protocol_t::write_response_t write(typename protocol_t::write_t write, order_token_t tok) {
        std::vector<typename protocol_t::region_t> regions;
        std::vector<int> destinations;
        for (int i = 0; i < (int)timestampers.size(); i++) {
            if (timestampers[i].region.overlaps(write.get_region())) {
                regions.push_back(timestampers[i].region);
                destinations.push_back(i);
            }
        }
        std::vector<typename protocol_t::write_t> subwrites = write.shard(regions);
        rassert(subwrites.size() == regions.size());
        std::vector<typename protocol_t::write_response_t> responses;
        for (int i = 0; i < (int)regions.size(); i++) {
            rassert(regions[i].contains(subwrites[i].get_region()));
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
    static void create(std::string path, std::vector<typename protocol_t::region_t> shards, int repli_factor) {

        log_serializer_t::create(
            log_serializer_t::dynamic_config_t(),
            log_serializer_t::private_dynamic_config_t(path),
            log_serializer_t::static_config_t()
            );

        log_serializer_t serializer(
            /* Extra parentheses are necessary so C++ doesn't interpret this as
            a declaration of a function called `serializer`. WTF, C++? */
            (log_serializer_t::dynamic_config_t()),
            log_serializer_t::private_dynamic_config_t(path)
            );

        std::vector<serializer_t *> multiplexer_files;
        multiplexer_files.push_back(&serializer);

        serializer_multiplexer_t::create(multiplexer_files, shards.size() * repli_factor);

        serializer_multiplexer_t multiplexer(multiplexer_files);

        for (int i = 0; i < (int)shards.size(); i++) {
            for (int j = 0; j < repli_factor; j++) {
                protocol_t::store_t::create(multiplexer.proxies[i*repli_factor+j], shards[i]);
            }
        }
    }

    dummy_namespace_interface_t(std::string path, std::vector<typename protocol_t::region_t> shards, int repli_factor) {

        serializer.reset(new log_serializer_t(
            log_serializer_t::dynamic_config_t(),
            log_serializer_t::private_dynamic_config_t(path)
            ));

        std::vector<serializer_t *> multiplexer_files;
        multiplexer_files.push_back(serializer.get());

        multiplexer.reset(new serializer_multiplexer_t(multiplexer_files));

        std::vector<typename dummy_sharder_t<protocol_t>::timestamper_and_region_t> shards_of_this_db;

        for (int i = 0; i < (int)shards.size(); i++) {

            std::vector<typename protocol_t::store_t *> mirrors_of_this_shard;

            for (int j = 0; j < repli_factor; j++) {
                typename protocol_t::store_t *store =
                    new typename protocol_t::store_t(multiplexer->proxies[i*repli_factor+j], shards[i]);
                stores.push_back(store);
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
    boost::scoped_ptr<log_serializer_t> serializer;
    boost::scoped_ptr<serializer_multiplexer_t> multiplexer;
    boost::ptr_vector<typename protocol_t::store_t> stores;
    boost::ptr_vector<dummy_parallelizer_t<protocol_t> > parallelizers;
    boost::ptr_vector<dummy_timestamper_t<protocol_t> > timestampers;
    boost::scoped_ptr<dummy_sharder_t<protocol_t> > sharder;
};

#endif /* __SERVER_DUMMY_NAMESPACE_INTERFACE_HPP__ */
