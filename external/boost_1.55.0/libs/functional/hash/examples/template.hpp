
// Copyright 2012 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is an example of how to write a hash function for a template
// class.

#include <boost/functional/hash_fwd.hpp>

template <typename A, typename B>
class my_pair
{
    A value1;
    B value2;
public:
    my_pair(A const& v1, B const& v2)
        : value1(v1), value2(v2)
    {}

    bool operator==(my_pair const& other) const
    {
        return value1 == other.value1 &&
            value2 == other.value2;
    }

    friend std::size_t hash_value(my_pair const& p)
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.value1);
        boost::hash_combine(seed, p.value2);

        return seed;
    }
};

