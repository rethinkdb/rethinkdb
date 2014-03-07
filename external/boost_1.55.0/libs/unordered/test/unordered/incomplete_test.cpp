
// Copyright 2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../helpers/prefix.hpp"
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "../helpers/postfix.hpp"

#include <utility>

namespace x
{
    struct D { boost::unordered_map<D, D> x; };
}

namespace incomplete_test
{
    // Declare, but don't define some types.

    struct value;
    struct hash;
    struct equals;
    template <class T> struct allocator;

    // Declare some instances
    
    typedef boost::unordered_map<value, value, hash, equals,
        allocator<std::pair<value const, value> > > map;
    typedef boost::unordered_multimap<value, value, hash, equals,
        allocator<std::pair<value const, value> > > multimap;
    typedef boost::unordered_set<value, hash, equals,
        allocator<value> > set;
    typedef boost::unordered_multiset<value, hash, equals,
        allocator<value> > multiset;
    
    // Now define the types which are stored as members, as they are needed for
    // declaring struct members.

    struct hash { 
        template <typename T>
        std::size_t operator()(T const&) const { return 0; }
    };

    struct equals {
        template <typename T>
        bool operator()(T const&, T const&) const { return true; }
    };

    // This is a dubious way to implement an allocator, but good enough
    // for this test.
    template <typename T>
    struct allocator : std::allocator<T> {
        allocator() {}

        template <typename T2>
        allocator(const allocator<T2>& other) :
            std::allocator<T>(other) {}
    };

    // Declare some members of a structs.
    //
    // Incomplete hash, equals and allocator aren't here supported at the
    // moment.
    
    struct struct1 {
        boost::unordered_map<struct1, struct1, hash, equals,
            allocator<std::pair<struct1 const, struct1> > > x;
    };
    struct struct2 {
        boost::unordered_multimap<struct2, struct2, hash, equals,
            allocator<std::pair<struct2 const, struct2> > > x;
    };
    struct struct3 {
        boost::unordered_set<struct3, hash, equals,
            allocator<struct3> > x;
    };
    struct struct4 {
        boost::unordered_multiset<struct4, hash, equals,
            allocator<struct4> > x;
    };
    
    // Now define the value type.

    struct value {};

    // Create some instances.
    
    incomplete_test::map m1;
    incomplete_test::multimap m2;
    incomplete_test::set s1;
    incomplete_test::multiset s2;

    incomplete_test::struct1 c1;
    incomplete_test::struct2 c2;
    incomplete_test::struct3 c3;
    incomplete_test::struct4 c4;

    // Now declare, but don't define, the operators required for comparing
    // elements.

    std::size_t hash_value(value const&);
    bool operator==(value const&, value const&);

    std::size_t hash_value(struct1 const&);
    std::size_t hash_value(struct2 const&);
    std::size_t hash_value(struct3 const&);
    std::size_t hash_value(struct4 const&);
    
    bool operator==(struct1 const&, struct1 const&);
    bool operator==(struct2 const&, struct2 const&);
    bool operator==(struct3 const&, struct3 const&);
    bool operator==(struct4 const&, struct4 const&);
    
    // And finally use these
    
    void use_types()
    {
        incomplete_test::value x;
        m1[x] = x;
        m2.insert(std::make_pair(x, x));
        s1.insert(x);
        s2.insert(x);

        c1.x.insert(std::make_pair(c1, c1));
        c2.x.insert(std::make_pair(c2, c2));
        c3.x.insert(c3);
        c4.x.insert(c4);
    }

    // And finally define the operators required for comparing elements.

    std::size_t hash_value(value const&) { return 0; }
    bool operator==(value const&, value const&) { return true; }

    std::size_t hash_value(struct1 const&) { return 0; }
    std::size_t hash_value(struct2 const&) { return 0; }
    std::size_t hash_value(struct3 const&) { return 0; }
    std::size_t hash_value(struct4 const&) { return 0; }
    
    bool operator==(struct1 const&, struct1 const&) { return true; }
    bool operator==(struct2 const&, struct2 const&) { return true; }
    bool operator==(struct3 const&, struct3 const&) { return true; }
    bool operator==(struct4 const&, struct4 const&) { return true; }
}

int main() {
    // This could just be a compile test, but I like to be able to run these
    // things. It's probably irrational, but I find it reassuring.

    incomplete_test::use_types();
}
