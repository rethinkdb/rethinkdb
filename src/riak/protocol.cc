#include "riak/protocol.hpp"
#include "utils.hpp"

namespace riak {
region_t::region_t(key_spec_t _key_spec) 
    : key_spec(_key_spec)
{ }

bool region_t::_contains(finite_t x, finite_t y) { 
    return all_in_container_match_predicate(y, boost::bind(&std_contains<finite_t, std::string>, x, _1));
}
bool region_t::_contains(finite_t, hash_range_t y) {
    //notice, hash_range_ts are either infinite or empty, thus if y isn't empty
    //it can't possibly be contains in a finite range
    return y.first == y.second;
}

bool region_t::_contains(hash_range_t x, finite_t y) {
    return all_in_container_match_predicate(y, boost::bind(&region_t::hash_range_contains_key, x, _1));
}

bool region_t::_contains(hash_range_t x, hash_range_t y) {
    return (y.first >= x.first && y.second <= x.second);
}

bool region_t::_overlaps(finite_t x, finite_t y) {
    return all_in_container_match_predicate(y, boost::bind(&std_does_not_contain<finite_t, std::string>, x, _1));
}

bool region_t::_overlaps(finite_t x, hash_range_t y) {
    return !all_in_container_match_predicate(x, boost::bind(&notf, boost::bind(&region_t::hash_range_contains_key, y, _1)));
}

bool region_t::_overlaps(hash_range_t x, finite_t y) {
    return _overlaps(y, x); //overlaps is commutative
}

bool region_t::_overlaps(hash_range_t x, hash_range_t y) {
    return hash_range_contains_hash(x, y.first)  ||
           hash_range_contains_hash(x, y.second) ||
           hash_range_contains_hash(y, x.first);
}

region_t::key_spec_t region_t::_intersection(finite_t x, finite_t y) {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (std_contains(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::_intersection(finite_t x, hash_range_t y) {
    finite_t res;
    for (key_it_t it = x.begin(); it != x.end(); it++) {
        if (hash_range_contains_key(y, *it)) {
            res.insert(*it);
        }
    }

    return res;
}

region_t::key_spec_t region_t::_intersection(hash_range_t x, finite_t y) {
    return _intersection(y,x);
}

region_t::key_spec_t region_t::_intersection(hash_range_t x, hash_range_t y) {
    if (_overlaps(x,y)) {
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


/* bool region_t::contains(const region_t &other) const {
    for (std::set<std::string>::const_iterator it = other.keys.begin(); it != other.keys.end(); it++) {
        if (!std_contains(keys, *it)) { 
            return false; 
        }
    }
    return true;
}

bool region_t::overlaps(const region_t &other) const {
    for (std::set<std::string>::const_iterator it = other.keys.being(); it != other.keys.end(); it++) {
        if (std_contains(keys, *it)) {
            return true;
        }
    }
    return false;
}

region_t region_t::intersection(const region_t &other) const {
    region_t res;

    for (std::set<std::string>::const_iterator it = other.keys.begin(); it != other.keys.end(); it++) {
        if (std_contains(keys, *it)) {
            res.insert(*it);
        }
    }

    return res;
} */
} //namespace riak 
