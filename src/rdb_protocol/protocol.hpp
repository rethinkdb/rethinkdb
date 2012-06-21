#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include "utils.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/variant.hpp"

#include "btree/keys.hpp"
#include "http/json.hpp"
#include "http/json/cJSON.hpp"

struct rdb_protocol_t {
    typedef key_range_t region_t;

    class point_read_response_t {
    public:
        boost::shared_ptr<scoped_cJSON_t> data;
    };

    class read_response_t {
    public:
        boost::variant<point_read_response_t> response;
    };

    class point_read_t {
    public:
        point_read_t() { }
        explicit point_read_t(const store_key_t& key_) : key(key_) { }

        store_key_t key;
    };

    class read_t {
    public:
        boost::variant<point_read_t> read;
    };

    class point_write_response_t {
        enum {
            STORED,
            DUPLICATE
        } result;
    };

    class write_response_t {
        boost::variant<point_write_response_t> response;
    };

    class point_write_t {
    public:
        point_write_t() { }
        point_write_t(const store_key_t& key_, boost::shared_ptr<scoped_cJSON_t> data_)
            : key(key_), data(data_) { }

        store_key_t key;
        boost::shared_ptr<scoped_cJSON_t> data;
    };

    class write_t {
    public:
        boost::variant<point_write_t> write;
    };
};

#endif  // RDB_PROTOCOL_PROTOCOL_HPP_
