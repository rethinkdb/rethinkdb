#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include "btree/keys.hpp"
#include "containers/clone_ptr.hpp"
#include "http/json.hpp"
#include <boost/variant.hpp>

enum point_write_result_t {
    STORED,
    DUPLICATE
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(point_write_result_t, int8_t, STORED, DUPLICATE);

struct rdb_protocol_t {
    typedef key_range_t region_t;

    struct temporary_cache_t { };

    struct point_read_response_t {
        clone_ptr_t<cJSON> data;
        point_read_response_t(clone_ptr_t<cJSON> _data) 
            : data(_data)
        { }

        RDB_MAKE_ME_SERIALIZABLE_1(data);
    };

    struct read_response_t {
        boost::variant<point_read_response_t> response;

        read_response_t() { }
        read_response_t(const read_response_t& r) : response(r.response) { }
        explicit read_response_t(const point_read_response_t& r) : response(r) { }
        key_range_t get_region() const THROWS_NOTHING;

        RDB_MAKE_ME_SERIALIZABLE_1(response);
    };

    struct point_read_t {
        store_key_t key;

        RDB_MAKE_ME_SERIALIZABLE_1(key);
    };

    struct read_t {
        boost::variant<point_read_t> read;

        key_range_t get_region() const THROWS_NOTHING;
        read_t shard(const key_range_t &region) const THROWS_NOTHING;
        read_response_t unshard(std::vector<read_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        read_t() { }
        read_t(const read_t& r) : read(r.read) { }
        read_t(const point_read_t &r) : read(r) { }

        RDB_MAKE_ME_SERIALIZABLE_1(read);
    };

    struct point_write_response_t {
        point_write_result_t result;

        RDB_MAKE_ME_SERIALIZABLE_1(result);
    };

    struct write_response_t {
        boost::variant<point_write_response_t> response;

        write_response_t() { }
        write_response_t(const write_response_t& w) : response(w.response) { }
        explicit write_response_t(const point_write_response_t& w) : response(w) { }

        RDB_MAKE_ME_SERIALIZABLE_1(response);
    };

    struct point_write_t {
        store_key_t key;
        clone_ptr_t<cJSON> data;

        RDB_MAKE_ME_SERIALIZABLE_2(key, data);
    };

    struct write_t {
        boost::variant<point_write_t> write;

        key_range_t get_region() const THROWS_NOTHING;
        write_t shard(key_range_t region) const THROWS_NOTHING;
        write_response_t unshard(std::vector<write_response_t> responses, temporary_cache_t *cache) const THROWS_NOTHING;

        write_t() { }
        write_t(const write_t& w) : write(w.write) { }
        write_t(const point_write_t &w) : write(w) { }

        RDB_MAKE_ME_SERIALIZABLE_1(write);
    };
};

#endif
