#include "rdb_protocol/protocol.hpp"

typedef rdb_protocol_t::read_t read_t;
typedef rdb_protocol_t::read_response_t read_response_t;

typedef rdb_protocol_t::point_read_t point_read_t;
typedef rdb_protocol_t::point_read_response_t point_read_response_t;

typedef rdb_protocol_t::write_t write_t;
typedef rdb_protocol_t::write_response_t write_response_t;

typedef rdb_protocol_t::point_write_t point_write_t;
typedef rdb_protocol_t::point_write_response_t point_write_response_t;


/* read_t::get_region implementation */
struct r_get_region_visitor : public boost::static_visitor<key_range_t> {
    key_range_t operator()(const point_read_t &pr) const {
        return key_range_t(pr.key);
    }
};

key_range_t read_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(r_get_region_visitor(), read);
}

/* read_t::shard implementation */

struct r_shard_visitor : public boost::static_visitor<read_t> {
    r_shard_visitor(const key_range_t &_key_range)
        : key_range(_key_range)
    { }

    read_t operator()(const point_read_t &pr) const {
        rassert(key_range_t(pr.key) == key_range);
        return read_t(pr);
    }
    const key_range_t &key_range;
};

read_t read_t::shard(const key_range_t &region) const THROWS_NOTHING {
    return boost::apply_visitor(r_shard_visitor(region), read);
}

/* read_t::unshard implementation */

read_response_t rdb_protocol_t::read_t::unshard(std::vector<read_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    return responses[0];
}

/* write_t::get_region() implementation */

struct w_get_region_visitor : public boost::static_visitor<key_range_t> {
    key_range_t operator()(const point_write_t &pw) const {
        return key_range_t(pw.key);
    }
};

key_range_t write_t::get_region() const THROWS_NOTHING {
    return boost::apply_visitor(w_get_region_visitor(), write);
}

/* write_t::shard implementation */

struct w_shard_visitor : public boost::static_visitor<write_t> {
    w_shard_visitor(const key_range_t &_key_range)
        : key_range(_key_range)
    { }

    write_t operator()(const point_write_t &pw) const {
        rassert(key_range_t(pw.key) == key_range);
        return write_t(pw);
    }
    const key_range_t &key_range;
};

write_t write_t::shard(key_range_t region) const THROWS_NOTHING {
    return boost::apply_visitor(w_shard_visitor(region), write);
}

write_response_t write_t::unshard(std::vector<write_response_t> responses, temporary_cache_t *) const THROWS_NOTHING {
    rassert(responses.size() == 1);
    return responses[0];
}
