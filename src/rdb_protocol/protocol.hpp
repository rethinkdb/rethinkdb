#ifndef RDB_PROTOCOL_PROTOCOL_HPP_
#define RDB_PROTOCOL_PROTOCOL_HPP_

#include "btree/keys.hpp"

struct rdb_protocol_t {
    typedef key_range_t region_t;

    class point_read_response_t {
        clone_ptr_t<cJSON> data;
    };

    class read_response_t {
        boost::variant<point_read_response_t> response;
    };

    class point_read_t {
        store_key_t key;
    };

    class read_t {
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
        store_key_t key;
        clone_ptr_t<cJSON> data;
    };

    class write_t {
        boost::variant<point_write_t> write;
    };
};

#endif
