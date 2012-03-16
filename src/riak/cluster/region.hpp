#ifndef RIAK_CLUSTER_REGION_HPP_
#define RIAK_CLUSTER_REGION_HPP_

#include <string>
#include <set>
#include <boost/variant.hpp>
#include "errors.hpp"

namespace riak {

/* a region defines a subset (non-strict) of a keyspace where the keyspace is just the set of strings */

class region_t {
private:
    unsigned hash(std::string) { crash("Not implemented"); }
public:
    typedef std::set<std::string> finite_t;
    typedef std::pair<unsigned, unsigned> hash_range_t; //keys which hash to something in the range, [first, second)

    //universe_t and null_t are never implemented anywhere because they
    //have no implementation null sets and universe sets are all the same 
    //class universe_t {};
    //class null_t {};
public:
    static bool hash_range_contains_key(hash_range_t, std::string) { crash("Not implemented"); }
    static bool hash_range_contains_hash(hash_range_t, unsigned) { crash("Not implemented"); }

    /* regions can define the subset in one of 3 ways:
     * - as an finite set of keys
     * - as a range of hash values
     */
    typedef boost::variant<finite_t, hash_range_t> key_spec_t;
    key_spec_t key_spec;

public:
    explicit region_t(key_spec_t);

public:
    typedef finite_t::const_iterator key_it_t;

    //does x contain y
    
    struct contains_functor : public boost::static_visitor<bool> {
        bool operator()(const finite_t &, const finite_t &) const;
        bool operator()(const finite_t &, const hash_range_t &) const;
        bool operator()(const hash_range_t &, const finite_t &) const;
        bool operator()(const hash_range_t &, const hash_range_t &) const;
    };

    struct overlaps_functor : public boost::static_visitor<bool> {
        bool operator()(const finite_t &, const finite_t &) const;
        bool operator()(const finite_t &, const hash_range_t &) const;
        bool operator()(const hash_range_t &, const finite_t &) const;
        bool operator()(const hash_range_t &, const hash_range_t &) const;
    };

    struct intersection_functor : public boost::static_visitor<key_spec_t> {
        key_spec_t operator()(const finite_t &, const finite_t &) const;
        key_spec_t operator()(const finite_t &, const hash_range_t &) const;
        key_spec_t operator()(const hash_range_t &, const finite_t &) const;
        key_spec_t operator()(const hash_range_t &, const hash_range_t &) const;
    };

public:
    bool contains(region_t &);
    bool overlaps(region_t &);
    region_t intersection(region_t &);

public:
    static region_t universe();
    static region_t null();
};

} //namespace riak

#endif
