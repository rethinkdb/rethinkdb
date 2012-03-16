#ifndef RIAK_CLUSTER_READ_HPP_
#define RIAK_CLUSTER_READ_HPP_

#include <string>
#include <boost/optional.hpp>
#include "riak/cluster/region.hpp"
#include <vector>
#include "riak/structures.hpp"

namespace riak {
class temporary_cache;

class read_t;

class point_read_t;
class point_read_response_t;

class bucket_read_t;
class bucket_read_response_t;

class mapred_read_t;
class mapred_read_response_t;


/* a point read represents reading a single key from the database */
class point_read_t {
private:
    friend class read_functor;
    std::string key;
    boost::optional<std::pair<int, int> > range;
public:
    //point_read_t() { crash("Not implemented"); }
    explicit point_read_t(std::string);
    point_read_t(std::string, std::pair<int, int>);
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
};

class point_read_response_t {
public:
    explicit point_read_response_t(object_t _result) : result(_result) { }
private:
    object_t result;
};

/* bucket read represents reading all of the data from a bucket */
class bucket_read_t {
private:
    region_t region_limit;
public:
    bucket_read_t() : region_limit(region_t::universe()) { crash("Not implemented"); }
    explicit bucket_read_t(region_t _region_limit) : region_limit(_region_limit) { crash("Not implemented"); }
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
    bucket_read_response_t combine(const bucket_read_response_t &, const bucket_read_response_t &) const;
};

class bucket_read_response_t {
public:
    bucket_read_response_t() { }

    explicit bucket_read_response_t(object_iterator_t obj_it) {
        while (boost::optional<object_t> cur = obj_it.next()) {
            keys.push_back(cur->key);
        }
    }
    bucket_read_response_t(const bucket_read_response_t &x, const bucket_read_response_t &y) {
        keys.insert(keys.end(), x.keys.begin(), x.keys.end());
        keys.insert(keys.end(), y.keys.begin(), y.keys.end());
    }
private:
    std::vector<std::string> keys;
};

/* mapred reads read a subset of the data */

class mapred_read_t {
private:
    region_t region;
public:
    explicit mapred_read_t(region_t _region) : region(_region) {}
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t> regions);
};

class mapred_read_response_t {
public:
    static mapred_read_response_t unshard(std::vector<mapred_read_response_t> response, temporary_cache *);
    explicit mapred_read_response_t(std::string _json_result)
        : json_result(_json_result)
    { }
    /* mapred_read_response_t(const mapred_read_response_t &, const mapred_read_respone_t &) {
        crash("Not implemented");
    } */
private:
    std::string json_result;
};

typedef boost::variant<point_read_t, bucket_read_t, mapred_read_t> read_variant_t;
typedef boost::variant<point_read_response_t, bucket_read_response_t, mapred_read_response_t> read_response_variant_t;

typedef read_response_variant_t read_response_t;

class read_t {
public:
    read_variant_t internal;
public:
    explicit read_t(point_read_t r) : internal(r) {}
    explicit read_t(bucket_read_t r) : internal(r) {}
    explicit read_t(mapred_read_t r) : internal(r) {}
public:
    region_t get_region() const;
    std::vector<read_t> shard(std::vector<region_t>);
    read_response_t unshard(std::vector<read_response_t>);
};
} //namespace riak

#endif
