#include "riak/protocol.hpp"
#include "utils.hpp"

namespace riak {
region_t::region_t(key_spec_t _key_spec) 
    : key_spec(_key_spec)
{ }

bool region_t::contains_functor::operator()(const finite_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&std_contains<finite_t, std::string>, x, _1));
}
bool region_t::contains_functor::operator()(const finite_t &, const hash_range_t & y) const {
    //notice, hash_range_ts are either infinite or empty, thus if y isn't empty
    //it can't possibly be contains in a finite range
    return y.first == y.second;
}

bool region_t::contains_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&region_t::hash_range_contains_key, x, _1));
}

bool region_t::contains_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    return (y.first >= x.first && y.second <= x.second);
}

bool region_t::overlaps_functor::operator()(const finite_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&std_does_not_contain<finite_t, std::string>, x, _1));
}

bool region_t::overlaps_functor::operator()(const finite_t & x, const hash_range_t & y) const {
    return !all_in_container_match_predicate(x, boost::bind(&notf, boost::bind(&region_t::hash_range_contains_key, y, _1)));
}

bool region_t::overlaps_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return operator()(y, x); //overlaps is commutative
}

bool region_t::overlaps_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    return hash_range_contains_hash(x, y.first)  ||
           hash_range_contains_hash(x, y.second) ||
           hash_range_contains_hash(y, x.first);
}

region_t::key_spec_t region_t::intersection_functor::operator()(const finite_t & x, const finite_t & y) const {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (std_contains(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::intersection_functor::operator()(const finite_t & x, const hash_range_t & y) const {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (hash_range_contains_key(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::intersection_functor::operator()(const hash_range_t & x, const finite_t & y) const {
    return operator()(y,x);
}

region_t::key_spec_t region_t::intersection_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    if (overlaps_functor()(x,y)) {
        return std::make_pair(std::max(x.first, y.first), std::min(x.second, y.second));
    } else {
        return finite_t();
    }
}

region_t region_t::universe() {
    return region_t(std::make_pair(unsigned(0), ~unsigned(0)));
}

region_t region_t::null() {
    return region_t(std::make_pair(unsigned(0), unsigned(0)));
}

bool region_t::contains(region_t &other) {
    return boost::apply_visitor(contains_functor(), key_spec, other.key_spec);
}

bool region_t::overlaps(region_t &other) {
    overlaps_functor visitor;
    return boost::apply_visitor(visitor, key_spec, other.key_spec);
}

region_t region_t::intersection(region_t &other) {
    return region_t(boost::apply_visitor(intersection_functor(), key_spec, other.key_spec));
}

point_read_t::point_read_t(std::string _key) 
    : key(_key)
{ }

point_read_t::point_read_t(std::string _key, std::pair<int, int> _range)
    : key(_key), range(_range)
{ }

region_t point_read_t::get_region() {
    region_t::finite_t set;
    set.insert(key);
    return region_t(set);
}

std::vector<read_t> point_read_t::shard(std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    region_t region = get_region();
    rassert(regions[0].overlaps(region));

    std::vector<read_t> res;
    res.push_back(*this);
    return res;
}

region_t bucket_read_t::get_region() {
    return region_limit;
}

std::vector<read_t> bucket_read_t::shard(std::vector<region_t> regions) {
    std::vector<read_t> res;

    for (std::vector<region_t>::iterator it = regions.begin(); it != regions.end(); it++) {
        res.push_back(read_t(bucket_read_t(*it)));
    }

    return res;
}

bucket_read_response_t bucket_read_response_t::unshard(std::vector<bucket_read_response_t> responses) {
    bucket_read_response_t res;
    for (std::vector<bucket_read_response_t>::iterator it = responses.begin(); it != responses.end(); it++) {
        res.keys.insert(res.keys.end(), it->keys.begin(), it->keys.end());
    }

    return res;
}

region_t mapred_read_t::get_region() {
    return region;
}

std::vector<read_t> mapred_read_t::shard(std::vector<region_t> regions) {
    std::vector<read_t> res;
    for (std::vector<region_t>::iterator it = regions.begin(); it != regions.end(); it++) {
        res.push_back(read_t(mapred_read_t(it->intersection(region))));
    }
    return res;
}

mapred_read_response_t mapred_read_response_t::unshard(std::vector<mapred_read_response_t>, unshard_ctx_t *) {
    crash("Not implemented");
}

read_response_t read_enactor_vistor_t::operator()(point_read_t read, riak_interface_t *riak) {
    if (read.range) {
        return read_response_t(point_read_response_t(riak->get_object(read.key, *(read.range))));
    } else {
        return read_response_t(point_read_response_t(riak->get_object(read.key)));
    }
}

read_response_t read_enactor_vistor_t::operator()(bucket_read_t, riak_interface_t *riak) {
    return read_response_t(bucket_read_response_t(riak->objects()));
}

read_response_t read_enactor_vistor_t::operator()(mapred_read_t, riak_interface_t *) {
    crash("Not implemented");
}

region_t write_t::get_region() {
    std::set<std::string> res;
    res.insert(key);
    return region_t(res);
}

std::vector<write_t> write_t::shard(std::vector<region_t> regions) {
    rassert(regions.size() == 1);
    rassert(get_region().overlaps(regions[0]));

    std::vector<write_t> res;
    res.push_back(*this);
    return res;
}

write_response_t write_response_t::unshard(std::vector<write_response_t> responses) {
    rassert(responses.size() == 1);

    return responses[0];
}

store_t::store_t(region_t _region, riak_interface_t *_interface)
    : region(_region), interface(_interface), backfilling(false)
{ }

region_t store_t::get_region() {
    return region;
}

bool store_t::is_coherent() {
    return !backfilling;
}

repli_timestamp_t store_t::get_timestamp() {
    crash("Not implemented");
}

read_response_t store_t::read(read_t , order_token_t ) {
    crash("Not implemented");
}

write_response_t store_t::write(write_t, repli_timestamp_t, order_token_t) {
    crash("Not implemented");
}

/* bool store_t::is_backfilling() {
    return backfilling;
}

void backfillee_chunk(backfill_chunk_t);

void backfillee_end(backfill_end_t);

void backfillee_cancel(); */

} //namespace riak 
