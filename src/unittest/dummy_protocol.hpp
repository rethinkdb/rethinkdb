#ifndef __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__
#define __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__

#include <vector>
#include <set>
#include <string>
#include <map>
#include "utils.hpp"
#include "concurrency/fifo_checker.hpp"

struct signal_t;

namespace unittest {

class dummy_protocol_t {

public:

    class region_t {

    public:
        bool contains(const region_t &r);
        bool overlaps(const region_t &r);
        region_t intersection(const region_t &r);

        std::set<std::string> keys;
    };

    class read_t {

    public:
        region_t get_region();
        std::vector<read_t> shard(std::vector<region_t> regions);
        std::vector<read_t> parallelize(int optimal_factor);

        region_t keys;
    };

    class read_response_t {

    public:
        static read_response_t unshard(std::vector<read_response_t> resps);
        static read_response_t unparallelize(std::vector<read_response_t> resps);

        std::map<std::string, std::string> values;
    };

    class write_t {

    public:
        region_t get_region();
        std::vector<write_t> shard(std::vector<region_t> regions);

        std::map<std::string, std::string> values;
    };

    class write_response_t {

    public:
        static write_response_t unshard(std::vector<write_response_t> resps);

        std::map<std::string, std::string> old_values;
    };

    class store_t {

    public:
        region_t get_region();
        bool is_coherent();
        repli_timestamp_t get_timestamp();

        read_response_t read(read_t read, order_token_t otok);
        write_response_t write(write_t write, repli_timestamp_t timestamp, order_token_t otok);

        bool is_backfilling();

        struct backfill_request_t {
            region_t region;
            repli_timestamp_t earliest, latest;
        };

        struct backfill_chunk_t {
            std::string key, value;
            repli_timestamp_t timestamp;
        };

        struct backfill_end_t {
            repli_timestamp_t timestamp;
        };

        backfill_request_t backfillee_begin();
        void backfillee_chunk(backfill_chunk_t chunk);
        void backfillee_end(backfill_end_t end);
        void backfillee_cancel();

        backfill_end_t backfiller(backfill_request_t request,
            boost::function<void(backfill_chunk_t)> chunk_fun,
            signal_t *interruptor);

        /* This stuff isn't part of the protocol interface, but it's public for
        the convenience of the people using `dummy_protocol_t`. */

        store_t(region_t r);
        ~store_t();

        region_t region;

        bool coherent, backfilling;

        repli_timestamp_t earliest, latest;

        order_sink_t order_sink;

        std::map<std::string, std::string> values;
        std::map<std::string, repli_timestamp_t> timestamps;
    };
};

bool operator==(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);
bool operator!=(dummy_protocol_t::region_t a, dummy_protocol_t::region_t b);

}   /* namespace unittest */

#endif /* __UNITTEST_DUMMY_NAMESPACE_PROTOCOL_HPP__ */
