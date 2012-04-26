#include "errors.hpp"
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "riak/cluster/region.hpp"
#include "stl_utils.hpp"
#include "utils.hpp"

namespace riak {

region_t::region_t(key_spec_t _key_spec)
    : key_spec(_key_spec)
{ }

bool region_t::contains_functor::operator()(const finite_t & x, const finite_t & y) const {
    return all_in_container_match_predicate(y, boost::bind(&std_contains<finite_t>, x, _1));
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
    return all_in_container_match_predicate(y, boost::bind(&std_does_not_contain<finite_t>, x, _1));
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
    return operator()(y, x);
}

region_t::key_spec_t region_t::intersection_functor::operator()(const hash_range_t & x, const hash_range_t & y) const {
    if (overlaps_functor()(x, y)) {
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

} //namespace riak
