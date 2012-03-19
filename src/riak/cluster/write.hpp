#ifndef RIAK_CLUSTER_WRITE_HPP_
#define RIAK_CLUSTER_WRITE_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "riak/structures.hpp"
#include "riak/cluster/region.hpp"
#include "riak/riak_interface.hpp"

namespace riak {

class temporary_cache_t;

class write_t;

class set_write_t {
private:
    friend class write_functor;
    object_t object;
    boost::optional<etag_cond_spec_t> etag_cond_spec;
    boost::optional<time_cond_spec_t> time_cond_spec;
    bool return_body;

public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t> regions);
};

struct set_write_response_t {
    riak_interface_t::set_result_t result;
    //if a set is done without a key then we need to pick the key ourselves.
    //and tell them what we picked.
    boost::optional<std::string> key_if_created;

    //sometimes a set will ask that the object be sent back with the response. the object
    boost::optional<object_t> object_if_requested;
};

class delete_write_t {
private:
    friend class write_functor;
    std::string key;
public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t> regions);
};

struct delete_write_response_t {
    /* whether or not we actually found the write */
    bool found;
    explicit delete_write_response_t(bool _found) : found(_found) { }
};

typedef boost::variant<set_write_t, delete_write_t> write_variant_t;
typedef boost::variant<set_write_response_t, delete_write_response_t> write_response_variant_t;

typedef write_response_variant_t write_response_t;

class write_t {
public:
    write_variant_t internal;
public:
    explicit write_t(set_write_t _internal) : internal(_internal) { }
    explicit write_t(delete_write_t  _internal) : internal(_internal) { }
public:
    region_t get_region();
    std::vector<write_t> shard(std::vector<region_t>);
    write_response_t unshard(std::vector<write_response_t>, temporary_cache_t *);
};
} //namespace riak 

#endif
