
// Copyright 2005 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/functional/hash.hpp>
#include <cassert>

// This example illustrates how to use boost::hash_combine to generate a hash
// value from the different members of a class. For full details see the hash
// tutorial.

class point
{
    int x;
    int y;
public:
    point() : x(0), y(0) {}
    point(int x, int y) : x(x), y(y) {}

    bool operator==(point const& other) const
    {
        return x == other.x && y == other.y;
    }

    friend std::size_t hash_value(point const& p)
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.x);
        boost::hash_combine(seed, p.y);

        return seed;
    }
};

int main()
{
    boost::hash<point> point_hasher;

    point p1(0, 0);
    point p2(1, 2);
    point p3(4, 1);
    point p4 = p1;

    assert(point_hasher(p1) == point_hasher(p4));

    // These tests could legally fail, but if they did it'd be a pretty bad
    // hash function.
    assert(point_hasher(p1) != point_hasher(p2));
    assert(point_hasher(p1) != point_hasher(p3));

    return 0;
}

