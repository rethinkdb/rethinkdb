#ifndef __RIAK_PROTOCOL_HPP__
#define __RIAK_PROTOCOL_HPP__

#include <set>
#include <string>
#include <boost/variant.hpp>
#include "utils.hpp"
#include <boost/bind.hpp>

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
    private:
        static bool hash_range_contains_key(hash_range_t, std::string) { crash("Not implemented"); }
        static bool hash_range_contains_hash(hash_range_t, unsigned) { crash("Not implemented"); }

        /* regions can define the subset in one of 3 ways:
         * - as an finite set of keys
         * - as a range of hash values
         * - as the universe (all the keys);
         * - as null (none of the keys); 
         */
        typedef boost::variant<finite_t, hash_range_t> key_spec_t;
        key_spec_t key_spec;

    public:
        region_t(key_spec_t);

    private:
        typedef finite_t::const_iterator key_it_t;

        //does x contain y
        
        bool _contains(finite_t, finite_t);
        bool _contains(finite_t, hash_range_t);
        bool _contains(hash_range_t, finite_t);
        bool _contains(hash_range_t, hash_range_t);

        bool _overlaps(finite_t, finite_t);
        bool _overlaps(finite_t, hash_range_t);
        bool _overlaps(hash_range_t, finite_t);
        bool _overlaps(hash_range_t, hash_range_t);

        key_spec_t _intersection(finite_t, finite_t);
        key_spec_t _intersection(finite_t, hash_range_t);
        key_spec_t _intersection(hash_range_t, finite_t);
        key_spec_t _intersection(hash_range_t, hash_range_t);

    public:
        bool contains(const region_t &) const;
        bool overlaps(const region_t &) const;
        region_t intersection(const region_t &) const;

    public:
        static region_t universe();
        static region_t null();
    };

    /* 
    template <class T>
    region_t::_contains(T, universe_t) { return false; }

    template <>
    struct region_t::contains_functor<finite_t , finite_t> {
        return all_in_container_match_predicate(y, boost::bind(&std_contains<finite_t, std::string>, x, _1));
    }

    template <>
    bool region_t::_contains(finite_t, hash_range_t) {
        return false; //hash_ranges are infinite, thus not contained in
    } */

} //namespace riak 

#endif 
